#include <coroutine>
#include <iostream>
#include <thread>
#include <array>
#include <list>
#include <initializer_list>
#include <functional>
#include <string>

/// 本例中，我们来给Generator加上模板

template <typename T>
struct Generator
{
	class ExhaustedException : std::exception {};

	struct promise_type
	{
		T value{ 0 };
		bool is_ready = false;

		std::suspend_always initial_suspend() { return {}; };

		std::suspend_always final_suspend() noexcept { return {}; };

		std::suspend_always yield_value(T _value)
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

	T next()
	{
		if (has_next())
		{
			handle.promise().is_ready = false;
			return handle.promise().value;
		}
		throw ExhaustedException();
	}

	/// T []
	/// int array[] = {1, 2, 3, 4}
	/// auto gen = Generator<int>::from_array(array, 4)
	Generator static from_array(T array[], int n)
	{
		for (int i = 0; i < n; ++i)
		{
			co_yield array[i];
		}
	}

	/// list
	/// auto gen = Generator<int>::from_list(std::list{1, 2, 3, 4});
	Generator static from_list(std::list<T> list)
	{
		for (auto t : list)
		{
			co_yield t;
		}
	}

	/// initializer_list
	/// auto gen = Generator<int>::from({1, 2, 3, 4})
	Generator static from(std::initializer_list<T> args)
	{
		for (auto t : args)
		{
			co_yield t;
		}
	}

	/// fold expression C++17
	/// 注意这里不要用递归去展开参数模板包
	/// 不然会生成多个Generator对象
	template<typename ...TArgs>
	Generator static from(TArgs ...args)
	{
		(co_yield args, ...);
	}

	/// Monad
	/// map and flat_map
	/// map 就是将 Generator 当中的 T 映射成一个新的类型 U
	/// 得到一个新的 Generator<U> 
	template <typename U>
	Generator<U> map(std::function<U(T)> f)
	{
		/// 判断this当中是否有下一个元素
		while (has_next())
		{
			/// 通过 f 将其变成 U 类型的值，再使用 co_yield 传出
			co_yield f(next());
		}
	}

	/// 利用 std::invoke_result_t C++17
	/// std::invoke_result_t 可以用来获取调用给定函数对象挥着函数指针时的返回类型
	/// 它接受一个可调用对象（函数指针、函数对象、成员函数指针等）和一组参数类型
	template<typename F>
	Generator<std::invoke_result_t<F, T>> map(F f)
	{
		while (has_next())
		{
			/// 将 Generator 值的类型从 T 映射到 U
			co_yield f(next());
		}
	}


	/// flat_map
	/// 前面提到的 map 是元素到元素的映射，而 flap_map 是元素到 Generator 的映射
	/// 然后将这些映射之后的 Generator 再展开，组合成一个新的 Generator
	/// 例如如果一个Generator会传出5个值，那么这5个值每一个值都会映射成一个新的 Generator
	/// 得到的这5个Generator又会整合成一个新的 Generator
	
	template<typename F>
	std::invoke_result_t<F, T> flat_map(F f) /// 这里的这个返回值必须显示的写出来
	{
		while (has_next())
		{
			/// 值映射成新的 Generator
			auto gen = f(next());
			/// 将新的 Generator 展开
			while (gen.has_next())
			{
				/// 产生更多的 Generator
				co_yield gen.next();
			}
		}
	}

	/// map 和 flat_map 的区别就是对原来元素的处理逻辑不同

	/// 折叠函数，对多个值进行一个整体操作最终得到一个值
	template<typename R, typename F>
	R fold(R initial, F f)
	{
		R acc = initial;
		while (has_next())
		{
			acc = f(acc, next());
		}
		return acc;
	}

	/// 过滤函数
	template <typename F>
	Generator filter(F f)
	{
		while (has_next())
		{
			T value = next();
			if (f(value))
			{
				co_yield value;
			}
		}
	}

	/// 提取前n个值
	Generator take(int n)
	{
		int i = 0;
		while (i++ < n && has_next())
		{
			co_yield next();
		}
	}

	/// 提取到指定条件
	template <typename F>
	Generator take_while(F f)
	{
		while (has_next())
		{
			T value = next();
			if (f(value))
			{
				co_yield value;
			}
			else
			{
				break;
			}
		}
	}

	/// 遍历所有的值，消费生成的 Generator
	template<typename F>
	void for_each(F f)
	{
		while (has_next())
		{
			f(next());
		}
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



/// 现在我们知道 fibonacci 函数就是一个协程，Generator 就是链接协程内外promise_type的封装
/// 想要创建一个Generator就需要定义一个函数或者 Lambda
/// 不过从输出结果来看，Generator 就是一个懒序列，因此我们当然可以通过一个数组创建出 Generator
Generator<int> fibonacci()
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
	/// 方便理解 flat_map
	Generator<int>::from(1, 2, 3, 4)
		.flat_map([](auto i)->Generator<int> { /// 返回值类型必须显示写出来，表明这个函数是个协程
		for (int j = 0; j < i; ++j)
		{
			/// 原本的序列是 -> 1，2，3，4
			/// 经过 flat_map 后变成 -> 0，0 1，0 1 2，0 1 2 3
			co_yield j;
		}
			})
		.for_each([](auto i) {
		if (i == 0)
		{
			std::cout << std::endl;
		}
		std::cout << "* ";
			});

	/// 折叠函数求阶乘
	auto result = Generator<int>::from(1, 2, 3, 4, 5)
		.fold(1, [](auto acc, auto i) {
		return acc * i;
			});

	/// 函数的调用时机
	Generator<int>::from(1, 2, 3, 4, 5, 6, 7, 8, 9)
		.filter([](auto i) {
		return i & 1 == 1;
			})
		.map([](auto i) {
		return i * 3;
			})
		.flat_map([](auto i) ->Generator<int> {
		for (int j = 0; j < i; j++)
		{
			co_yield j;
		}
			})
		.take(3)
		.for_each([](auto i) {
		std::cout << i << std::endl;
			});

	return 0;
}