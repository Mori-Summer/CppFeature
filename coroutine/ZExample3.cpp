#include <coroutine>
#include <exception>
#include <optional>
#include <thread>
#include <mutex>
#include <list>
#include <iostream>

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

private:
	// optional 可以判断 m_tResult 是否有值，借此可以判断协程是否执行完毕
	std::optional<Result<T>> m_tResult;		// 存放结果

};

int main()
{
	std::cout << "good night" << std::endl;
}