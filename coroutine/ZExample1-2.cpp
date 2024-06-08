#include <coroutine>
#include <iostream>
#include <thread>

/// 本例中，将 co_await 修改成 co_yield
/// 需要修改的地方是将 await_transform 修改为 yield_value
/// 尽管可以实现相同的效果，但是通常情况下 co_await 更多的关注点在于挂起自己
/// 而 co_yield 则是挂起自己传值出去
/// 因此我们应该针对合适的场景做出合适的选择

struct Generator
{
	class ExhaustedException : std::exception {};

	struct promise_type
	{
		int value{ 0 };
		bool is_ready = false;

		std::suspend_always initial_suspend() { return {}; };

		std::suspend_always final_suspend() noexcept { return {}; };

		std::suspend_always yield_value(int _value)
		{
			this->value = _value;
			is_ready = true;
			return {};
		}

		/// co_yield 就不需要返回值了
		void return_void() {};

		void unhandled_exception() {};

		Generator get_return_object()
		{
			return Generator{ std::coroutine_handle<promise_type>::from_promise(*this) };
		}
	};

	bool has_next()
	{
		if (handle.done())
		{
			return false;
		}
		if (!handle.promise().is_ready)
		{
			handle.resume();
		}

		if (handle.done())
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	int next()
	{
		if (has_next())
		{
			handle.promise().is_ready = false;
			return handle.promise().value;
		}
		throw ExhaustedException();
	}

	explicit Generator(std::coroutine_handle<promise_type> handle) noexcept
		: handle(handle) {}

	Generator(Generator&& generator) noexcept
		: handle(std::exchange(generator.handle, {})) {}

	Generator(Generator&) = delete;
	Generator& operator=(Generator&) = delete;

	~Generator()
	{
		if (handle) handle.destroy();
	}

	std::coroutine_handle<promise_type> handle;
};


Generator fibonacci()
{
	co_yield 0;
	co_yield 1;
	
	int a = 0;
	int b = 1;

	while (true)
	{
		co_yield a + b;
		b = a + b;
		a = b - a;
	}
}

int main()
{
	auto gen = fibonacci();
	for (int i = 0; i < 10; ++i)
	{
		if (gen.has_next())
		{
			std::cout << gen.next() << std::endl;
		}
		else
		{
			break;
		}
	}
	return 0;
}