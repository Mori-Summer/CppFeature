#include <coroutine>
#include <exception>
#include <optional>
#include <thread>
#include <mutex>
#include <list>

/// 本例中，我们定义一个类型 Task 来作为协程的返回值
/// Task 类型可以用来封装任何返回结果的异步行为
/// 持续返回值的情况可能更合适使用序列生成器

/// 1. 需要一个结果类型来承载正常返回和异常抛出的情况
/// 2. 需要为 Task 定义相应的 promise_type 类型来支持 co_return 和 co_await
/// 3. 为 Task 实现获取结果的阻塞函数 get_result 或者用于获取返回值的回调 then
///     以及用于获取抛出异常的回调 catching


template <typename T>
struct Task;

template <typename T>
struct TaskAwaiter;


/// 这个类用来存放我们的结果
template <typename T>
struct Result
{
	/// 初始默认值
	explicit Result() = default;

	/// 当 Task 正常返回时，用其结果初始化 Result
	explicit Result(T&& value) 
		: _value(value) {}

	/// 当 Task 抛异常时，用异常初始化 Result
	explicit Result(std::exception_ptr&& exception_ptr)
		: _exception_ptr(exception_ptr) {};

	/// 读取结果，有异常则抛出异常
	T get_or_throw()
	{
		if (_exception_ptr)
		{
			std::rethrow_exception()
		}
		return _value;
	}


private:
	T _value{};
	std::exception_ptr _exception_ptr;

};

/// 这个类是协程的Promise协议
template <typename ResultType>
struct TaskPromise
{
	/// transform
	template<typename _ResultType>
	TaskAwaiter<_ResultType> await_transform(Task<_ResultType>&& task)
	{
		return TaskAwaiter<_ResultType>(std::move(task));
	}

	/// 协程立即执行
	std::suspend_never initial_suspend() { return {}; };

	/// 执行结束后挂起，等待外部销毁
	std::suspend_always final_suspend() { return {}; };

	/// 构造协程的返回值对象 Task
	Task<Result> get_return_object()
	{
		return Task{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
	}

	/// 将异常存入 result
	void unhandled_exception()
	{
		/// 同步阻塞获取结果
		std::lock_guard lock(completion_lock);
		result = Result<ResultType>(std::current_exception());
		/// 通知 get_result 当中的 wait
		completion.notify_all();
		/// 异步结果回调
		notify_callback();
	}

	/// 将返回值也存入result，对应协程内部的 co_return
	void result_value(ResultType value)
	{
		/// 同步阻塞获取结果
		std::lock_guard lock(completion_lock);
		result = Result<ResultType>(std::move(value));
		/// 通知 get_result 当中的wait
		completion.notify_all();
		/// 异步结果回调
		notify_callback();
	}

	ResultType get_result()
	{
		/// 如果 result 没有值，说明协程还没有完成，等待值被写入再返回
		std::unique_lock lock(completion_lock);
		if (!result.has_value())
		{
			/// 等待写入值之后调用 notify_all
			completion.wait(lock);
		}
		/// 如果有值，则直接返回或者抛出异常
		return result->get_or_throw();
	}

	void on_completed(std::function<void(Result<ResultType>)>&& func)
	{
		std::unique_lock lock(completion_lock);
		/// 加锁判断 result
		if (result.has_value())
		{
			/// result 已经有值了
			auto value = result.value();
			/// 解锁后再调用func，避免阻塞其他
			lock.unlock();
			func(value);
		}
		else
		{
			/// 否则添加回调函数，等待调用
			completion_callbacks.push_back(func);
		}
	}

private:
	void notify_callback()
	{
		auto value = result.value();
		for (auto& callback : completion_callbacks)
		{
			callback(value);
		}
		/// 调用完成，清空回调
		completion_callbacks.clear();
	}

private:
	/// 使用 std::optional 可以区分协程是否执行完毕
	/// 如果result没有值，optional会明确告诉我们
	std::optional<Result<ResultType>> result;

	/// 同步相关变量
	std::mutex completion_lock;
	std::condition_variable completion;

	/// 回调列表，我们允许对同一个 Task 添加多个回调
	std::list<std::function<void(Result<ResultType>)>> completion_callbacks;

};

/// 这个类是协程的Awaitable协议
template <typename T>
struct TaskAwaiter
{
	explicit TaskAwaiter(Task<T>&& task) noexcept
		: task(std::move(task)) {};

	TaskAwaiter(TaskAwaiter&& completion) noexcept
		: task(std::exchange(completion.task, {})) {};

	TaskAwaiter(TaskAwaiter&) = delete;
	TaskAwaiter& operator=(TaskAwaiter&) = delete;


	/// co_await 直接挂起
	constexpr bool await_ready() const noexcept
	{
		return false;
	}

	/// 挂起后执行此处
	void await_suspend(std::coroutine_handle<> handle) noexcept
	{
		/// 当 task 执行完之后调用 resume
		task.finally([handle]() {
			handle.resume();
			});
	}

	/// 协程恢复执行时，被等待的 Task 已经执行完，调用 get_result 来获取结果
	T await_resume() noexcept
	{
		return task.get_result();
	}

private:
	Task<T> task;
};

int main()
{
	return 0;
}