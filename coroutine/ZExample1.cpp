#include <coroutine>
#include <thread>
#include <chrono>
#include <future>
#include <iostream>

struct Generator
{
    /// 控制协程的生命周期和Generator的生命周期一致
    ~Generator()
    {
        handle.destroy();
    }
    /// 协程执行完成后，外部读取值时抛出异常
    class ExhaustedException : std::exception{};

    /// promise_type 是链接协程内外的桥梁，需要什么就找 promise_type要
    struct promise_type
    {
        /// co_await 表达式的值将会传递进来
        /// 这里我们传值的同时要挂起
        std::suspend_always await_transform(int _value)
        {
            this->value = _value;
            is_ready = true;
            return {};
        }

        /// 开始执行时直接挂起等待外部调用 resume 获取下一个值
        std::suspend_always initial_suspend() {return {};};

        /// 协程结束后（co_return执行后）挂起
        std::suspend_always final_suspend() noexcept {return {};};

        /// 这里简化处理，默认不会抛出异常
        void unhandled_exception() {};

        /// 协程返回值类型构建
        Generator get_return_object()
        {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        };

        /// 默认没有返回值，即 co_await 表达式没有返回值
        void return_void() {};

        /// 协程内部的返回值
        int value{};
        /// 标记协程当前是否有值
        bool is_ready = false;

    };

    /// has_next 被调用的时候有两种情况
    /// 1. 已经有一个值传出来了，还没有被外部消费
    /// 2. 还没有现成的值可以用，需要尝试恢复执行协程来看看还有没有下一个值传出来
    bool has_next()
    {
        /// 协程已经执行完毕
        if (handle.done())
        {
            return false;
        }
        /// 协程还没有执行完成，并且下一个值还没有准备好
        if (!handle.promise().is_ready)
        {
            handle.resume();
        }

        /// 恢复执行之后协程执行完，这时候必然没有通过 co_await 传出值来
        if (handle.done())
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    /// 获取协程内部的值
    int next()
    {
        if (has_next())
        {
            /// 此时就一定有值了，消费当前的值，并重置状态
            handle.promise().is_ready = false;
            return handle.promise().value;
        }
        throw ExhaustedException();
    }

    std::coroutine_handle<promise_type> handle;
};

/// 可以看到这个例子中，并没有满足 awaitable 条件的类型
/// 实际上对于 co_await <expr> 表达式中 expr 的处理，C++有完整的流程
/// 1. 如果 promise_type 中定义了 await_transform 函数，那么先通过
///    promise.await_transform(expr) 来对 expr 做一次转换，得到的对象称为 awaitable
///    否则 awaitable 就必须是 expr 本身，此时 expr 就必须满足 awaitable 条件
/// 2. 接下来使用 awaitable 对象获取 awaiter，如果 awaitable 对象有 operator co_await
///    那么等待体就是 operator co_await(awaitable)，否则等待体就是 awaitable 本身

/// 这里我们采用定义 await_transform 的方法
/// 因为 C++ 基础类型无法重载运算符


/// 协程对象
Generator sequence()
{
    int i = 0;
    while (i < 1)
    {
        co_await i++;
        int c = 0;
        c++;
    }
    int b = 0;
    b++;

    co_return;
    /// 这里会有一个隐式的 co_return
    /// co_return执行后协程就结束了，此时协程的状态取决于 final_suspend
    /// 如果 final_suspend 返回 std::suspend_never 则协程结束后不会挂起而是直接销毁
    /// 但是此时 Generator 对象还存在，理论上此时 Generator 对象内的 协程handle 指向的是一个无效的协程
    /// 所以一般为了让协程和Generator的生命周期一致，我们将 final_suspend 返回值设置为 std::suspend_always
    /// 协程结束后依然保持挂起状态，由Generator来控制协程的生命周期
}

int main()
{
    auto gen = sequence();
    for (int i = 0; i < 15; ++i)
    {
        if (gen.has_next()) {
            std::cout << gen.next() << std::endl;
        }
        else {
            std::cout << "M " << i << std::endl;
        }
    }
}

/// 本例中的协程流程
/// 1. auto gen = sequence() 调用
/// 2. Generator get_return_object() 返回值对象构建
/// 3. std::suspend_always initial_suspend() 协程开始执行，现在是直接挂起
/// 4. gen.has_next() -> handle.resume() 调用 resume() 让协程重新执行
/// 5. sequence() -> co_await i++ 执行协程sequence内的代码，执行到co_await
/// 6. std::suspend_always await_transform 将 i 转换成 awaiter 并再次挂起
/// 7. gen.next -> handle.promise().value 获取到返回值
/// 8. gen.has_next() -> handle.resume() 调用 resume() 让协程重新执行
/// 9. int c; c++ -> co_await i++ 执行协程sequence内逻辑，co_await结束到下一个co_await开始
/// 10. 重复第5步骤的流程 ...
/// 11. co_return 直到执行到协程sequence内的co_return
/// 12. std::suspend_always final_suspend() 调用结束函数，协程挂起，等待销毁destroy()