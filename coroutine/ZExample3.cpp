#include <coroutine>
#include <exception>
#include <optional>
#include <thread>
#include <mutex>
#include <list>
#include <iostream>
#include <functional>
#include <condition_variable>

/// 本例中，我们定义一个类型 Task 来作为协程的返回值
/// Task 类型可以用来封装任何返回结果的异步行为

/// 我们定义以 Task<ResultType> 为返回值类型的协程
/// 并且可以在协程内部使用 co_await 来等待其他 Task 的执行

/// 外部非协程内的函数访问 Task 的结果时，我们可以通过回调，或者同步阻塞调用两种方式

/// 由此我们可以得到：
///		1. 需要一个结果类型来承载正常返回和异常抛出的情况
///		2. 需要为 Task 定义相应的 promise_type 类型来支持 co_return 和 co_await
///		3. 为 Task 实现获取结果的阻塞函数 get_result 或者用于获取返回值的回调 then
///			以及用于获取抛出异常的回调 catching


/// 这个是我们的协程返回类型
/// 是一个通用的接口
template <typename T>
class Task;

/// 这个是我们的结果类型
/// 用来承载正常返回和异常抛出
template <typename T>
class Result;

/// 这个是协程的Awaitable协议类型
template <typename T>
class TaskAwaiter;

/// 这个是协程的Promise协议类型
template <typename T>
class TaskPromise;


/// ---------- Result ----------
template <typename T>
class Result
{
public:
	// 初始值默认
	explicit Result() = default;

	// 当 Task 正常返回的时候，我们初始化 m_tValue
	explicit Result(T&& value) : m_tValue(value) {};
	// 当 Task 抛异常的时候，我们初始化 m_pException
	explicit Result(std::exception_ptr&& e) : m_pException(e) {};

	// 获取结果，有异常则抛异常，没有则正常返回结果
	T GetOrThrow()
	{
		if (m_pException)
		{
			std::rethrow_exception(m_pException);
		}
		return m_tValue;
	}

private:
	T m_tValue{};												// 结果值
	std::exception_ptr m_pException{nullptr};	// 抛出的异常
};

/// ---------- Promise Type ----------
template <typename T>
class TaskPromise
{
public:
	// 协议接口
	std::suspend_never initial_suspend() { return {}; }
	std::suspend_always final_suspend() noexcept { return {}; }
	Task<T> get_return_object()
	{
		return Task{ std::coroutine_handle<TaskPromise>::from_promise(*this) };
	}
	void unhandled_exception()
	{
		// 存储异常
		std::lock_guard lock(m_lMutex);
		m_tResult = Result<T>(std::current_exception());
		// 通知 GetResult 中的 wait
		m_conCompletion.notify_all();
	}
	void return_value(T value)
	{
		// 存储返回值
		std::lock_guard lock(m_lMutex);
		m_tResult = Result<T>(std::move(value));
		// 通知 GetResult 中的 wait
		m_conCompletion.notify_all();
	}

	/// co_await 支持相关接口
	template <typename R>
	TaskAwaiter<R> await_transform(Task<R>&& task)
	{
		// 返回一个TaskAwaiter对象
		return TaskAwaiter<R>(std::move(task));
	}

	T GetResult()
	{
		// 如果 m_tResult 没有值，说明协程还没有运行完，等待值被写入后再返回
		std::unique_lock lock(m_lMutex);
		if (!m_tResult.has_value())
		{
			// 等待值写入后的 notify_all
			m_conCompletion.wait(lock);
		}
		// 如果有值，直接返回 (或抛异常)
		return m_tResult->GetOrThrow();
	}

	void OnCompleted(std::function<void(Result<T>)>&& func)
	{
		std::unique_lock lock(m_lMutex);
		if (m_tResult.has_value())
		{
			// 结果有值，则立即执行func
			auto value = m_tResult.value();
			lock.unlock();
			func(value);
		}
		else
		{
			// 否则则加入列表等待执行
			m_listCompletionCallbacks.push_back(func);
		}
	}

private:
	// 回调
	std::list<std::function<void(Result<T>)>> m_listCompletionCallbacks;
	
	// 回调通知
	void NotifyCallbacks()
	{
		auto value = m_tResult.value();
		for (auto& callback : m_listCompletionCallbacks)
		{
			callback(value);
		}
		// 调用完成，清空回调
		m_listCompletionCallbacks.clear();
	}

private:
	// 互斥量
	std::mutex m_lMutex;
	// 等待通知机制
	std::condition_variable m_conCompletion;

	// optional 可以判断 m_tResult 是否有值，借此可以判断协程是否执行完毕
	std::optional<Result<T>> m_tResult{};		// 存放结果

};

/// ---------- Awaitable ----------
template <typename T>
class TaskAwaiter
{
public:
	explicit TaskAwaiter(Task<T>&& task) noexcept
		: m_Task(std::move(task)) {};

	TaskAwaiter(TaskAwaiter&& completion) noexcept
		: m_Task(std::exchange(completion.m_Task, {})) {};

	TaskAwaiter(TaskAwaiter&) = delete;
	TaskAwaiter& operator=(TaskAwaiter) = delete;

	/// awaitable协议相关接口
	constexpr bool await_ready() const noexcept
	{
		/// co_await后立马挂起，然后走 await_suspend 的逻辑
		return false;
	}

	void await_suspend(std::coroutine_handle<> handle) noexcept
	{
		// task执行完后，协程重新运行
		m_Task.Finally([handle]() {
			handle.resume();
			});
	}

	T await_resume() noexcept
	{
		// 协程 resume 后执行到这里，返回task执行完后的值
		return m_Task.GetResult();
	}

private:
	Task<T> m_Task;
};

/// ---------- Task ----------
template <typename T>
class Task
{
public:
	// 协程协议
	using promise_type = TaskPromise<T>;

	T GetResult()
	{
		return m_coroHandle.promise().GetResult();
	}

	// 执行回调
	Task& Then(std::function<void(T)>&& func)
	{
		m_coroHandle.promise().OnCompleted([func](auto result) {
			try
			{
				func(result.GetOrThrow());
			}
			catch (std::exception& e)
			{

			}
			});
		return *this;
	}
	// 执行异常
	Task& Catching(std::function<void(std::exception&)>&& func)
	{
		m_coroHandle.promise().OnCompleted([func](auto result) {
			try
			{
				result.GetOrThrow();
			}
			catch (std::exception& e)
			{
				func(e);
			}
			});
		return *this;
	}

	Task& Finally(std::function<void()>&& func)
	{
		m_coroHandle.promise().OnCompleted([func](auto result) {
			func();
			});
		return *this;
	}

	explicit Task(std::coroutine_handle<promise_type> handle) noexcept
		: m_coroHandle(handle) {};

	Task(Task&& task) noexcept
		: m_coroHandle(std::exchange(task.m_coroHandle, {})) {};

	Task(Task&) = delete;
	Task& operator=(Task&) = delete;

	~Task()
	{
		if (m_coroHandle)
		{
			m_coroHandle.destroy();
		}
	}

private:
	// 协程句柄
	std::coroutine_handle<promise_type> m_coroHandle{};
};


Task<int> SimpleTask2()
{
	std::cout << "task 2 start" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "task 2 return after 1s" << std::endl;
	co_return 2;
}

Task<int> SimpleTask3()
{
	std::cout << "task 3 start" << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(2));
	std::cout << "task 3 return after 2s" << std::endl;
	co_return 3;
}

Task<int> SimpleTask()
{
	std::cout << "task start" << std::endl;
	auto result2 = co_await SimpleTask2();
	std::cout << "returns from task2: " << result2 << std::endl;
	auto result3 = co_await SimpleTask3();
	std::cout << "returns from task3: " << result3 << std::endl;
	co_return 1 + result2 + result3;
}

int main()
{
	auto simpleTask = SimpleTask();
	simpleTask.Then([](int i) {
		std::cout << "simpleTask end: " << i << std::endl;
		})
		.Catching([](std::exception& e) {
		std::cout << "error occurred: " << e.what() << std::endl;
			});
	try
	{
		auto i = simpleTask.GetResult();
		std::cout << "simple task end from get: " << i << std::endl;
	}
	catch (std::exception& e)
	{
		std::cout << "error: " << e.what() << std::endl;
	}
	return 0;
}