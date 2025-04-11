#include <iostream>
#include <coroutine>
#include <future>
#include <fstream>
#include <string>
#include <thread>

// awaiter
//      bool await_ready()
//      void await_suspend(std::coroutine_handle<> coro)
//      Type await_resume()

// promise_type
//      std::suspend_Type/awaiter initial_suspend()
//      std::suspend_Type/awaiter final_suspend()
//      std::suspend_Type/awaiter await_transform()
//      Class get_return_object() { Class{std::coroutine_handle<promise_type>::from_promise(*this)} }
//      void return_void()
//      void return_value(Type value)
//      void unhandled_exception()

