#include "ZCoroutine.h"

bool async_manual_reset_event::awaiter::await_ready() const noexcept
{
    /// 如果该事件已经被设置，则不需要挂起
    return m_event.is_set();
}

bool async_manual_reset_event::awaiter::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept
{
    /// 当前awaiter正在等待的event对象
    const void* const setState = &m_event;
    /// 当前协程的句柄
    m_awaitingCoroutine = awaitingCoroutine;
    /// 当前event对象的state
    void* oldValue = m_event.m_state.load(std::memory_order_acquire);

    do
    {
        /// 如果状态已经设置，则立刻恢复挂起状态
        if (oldValue == setState)
        {
            return false;
        }
        /// 如果当前状态没有设置，则将当前event对象的state中的event对象(上一个)加入链表
        m_next = static_cast<awaiter*>(oldValue);
    /// 然后将当前event的state设置为他本身
    } while (!m_event.m_state.compare_exchange_weak(
            oldValue,
            this,
            std::memory_order_release,
            std::memory_order_acquire
            ));

    /// 挂起当前协程
    return true;
    /// 整个链表的组装是通过 m_state 来链接的，并且每个新加入的值添加在链表头部
}

async_manual_reset_event::async_manual_reset_event(bool initiallySet) noexcept
: m_state(initiallySet ? this : nullptr)
{}

bool async_manual_reset_event::is_set() const noexcept
{
    return m_state.load(std::memory_order_acquire) == this;
}

void async_manual_reset_event::reset() noexcept
{
    void* oldValue = this;
    m_state.compare_exchange_strong(oldValue, nullptr, std::memory_order_acquire);
}

void async_manual_reset_event::set() noexcept
{
    /// exchange的作用是将当前atomic的值设置为this，并返回atomic之前的值
    /// 按照之前的链表规则取出awaiter
    void* oldValue = m_state.exchange(this, std::memory_order_acq_rel);
    if (oldValue != this)
    {
        auto* waiters = static_cast<awaiter*>(oldValue);
        while (waiters != nullptr)
        {
            auto* next = waiters->m_next;
            /// 重新激活挂起的协程
            waiters->m_awaitingCoroutine.resume();
            waiters = next;
        }
    }
}

async_manual_reset_event::awaiter
async_manual_reset_event::operator co_await() const noexcept
{
    return awaiter{*this};
}
















