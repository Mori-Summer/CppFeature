#include <coroutine>
#include <atomic>

/**
 * 可等待同步基元：异步手动复位事件
 * 该事件的基本要求是：多个并发执行的coroutine程序都需要等待该事件
 *                  等待时需要暂停等待中的coroutine程序，直到某个线程调用 set() 方法
 *                  然后恢复任何等待中的coroutine程序，如果某个线程已经调用了 set() 方法
 *                  则coroutine无须暂停
 */

/**
    类似于以下代码
    T value;
    async_manual_reset_event event;

    生产者一次调用即可产生一个值
    void producer()
    {
        value = some_long_running_computation();
        通过设置事件来发布值
        event.set()；
    }

    多个并发的消费者
    task<> consumer()
    {
        等待生产者发出信号 event.set()
        co_await event;
        现在可以安全的使用值
        std::cout << value << std::endl;
    }
*/

/**
 * 事件可能处于的状态：设置或未设置
 * 当事件处于 未设置 状态时，会有一个（可能是空）的等待coroutines列表，这些等待coroutines在等待事件变成设置状态
 * 当事件处于 设置 状态，不会有任何等待的 coroutines
 */

class async_manual_reset_event
{
    friend struct awaiter;
public:
    async_manual_reset_event(bool initiallySet = false) noexcept;

    /// No Copying/Moving
    async_manual_reset_event(const async_manual_reset_event&) = delete;
    async_manual_reset_event(async_manual_reset_event&&) = delete;
    async_manual_reset_event& operator=(const async_manual_reset_event&) = delete;
    async_manual_reset_event& operator=(async_manual_reset_event&&) = delete;

    bool is_set() const noexcept;

    struct awaiter;
    awaiter operator co_await() const noexcept;

    void set() noexcept;
    void reset() noexcept;

private:
    /// 存储一个地址，如果该地址等于this本身，则表示已经set
    /// 否则表示没有set
    mutable std::atomic<void*> m_state;
};

/**
 * awaiter需要知道要等待哪个async_manual_reset_event对象
 * 还需要充当等待者链表中的一个节点
 * 还需要存储正在执行 co_await 等待 coroutine 的 handle，以便恢复执行
 */
struct async_manual_reset_event::awaiter
{
    friend class async_manual_reset_event;

    /// 初始化awaiter等待的event对象
    awaiter(const async_manual_reset_event& event) noexcept
    : m_event(event)
    {}

    bool await_ready() const noexcept;
    bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
    /// 不需要返回值
    void await_resume() noexcept {};

private:
    const async_manual_reset_event& m_event;
    std::coroutine_handle<> m_awaitingCoroutine;
    awaiter* m_next;
};






















