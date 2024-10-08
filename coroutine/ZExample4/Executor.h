/// 调度器
/// 为了实现协程的异步调度逻辑，我们需要提供调度器的实现
/// 调度器实际上就是负责执行一段逻辑的代码

/// 调度的位置
/// 协程的本质就是挂起和恢复，因此想要实现调度，就必须在挂起和恢复上做文章
/// 那么答案只有一个：Awaiter
/// 其中 await_ready 和 await_resume 都要求同步返回
/// 所以我们仅剩的就是 await_suspend
/// 实际上，按照C++的设计，await_suspend 确实是用来提供调度支持的
/// 由于这个时间点协程已经完全挂起，因此我们可以在任意一个线程上调用 handle.resume()
/// 甚至不用担心线程安全的问题

/// 调度器的持有者
/// 调度器应该属于协程，这样的好处就是协程内部的代码均会被调度到它对应的调度器上执行
/// 可以确保逻辑的一致性和正确性
/// 所以这里我们放到 Promise Type 里面

