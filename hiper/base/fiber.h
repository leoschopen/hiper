/*
 * @Author: Leo
 * @Date: 2023-08-09 15:00:51
 * @LastEditTime: 2023-08-23 12:21:21
 * @Description: 协程
 */

#ifndef HIPER_FIBER_H
#define HIPER_FIBER_H

#include "thread.h"

#include <functional>
#include <memory>
#include <ucontext.h>

namespace hiper {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State
    {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    // 重置协程函数，并重置状态，利用已分配的內存继续使用
    void reset(std::function<void()> cb);

    // 切换到当前协程执行
    void swapIn();

    // 将当前协程切换到后台
    void swapOut();

    uint64_t getId() const { return id_; }
    State    getState() const { return state_; }

    void setState(State state) { state_ = state; }

    static void SetThis(Fiber* f);

    // 返回当前协程
    static Fiber::ptr GetThis();

    // 让出当前协程执行权，协程切换到后台，并切换状态
    static void YieldToReady();
    static void YieldToHold();

    // 总协程数
    static uint64_t TotalFibers();

    // 协程执行函数, 返回到主协程
    static void MainFunc();

    // 协程执行函数, 返回到调用协程
    static void CallerMainFunc();

    void call();

    void back();

    static uint64_t GetFiberId();

private:
    uint64_t   id_        = 0;
    uint32_t   stacksize_ = 0;
    State      state_     = INIT;
    ucontext_t ctx_;
    void*      stack_ = nullptr;

    std::function<void()> cb_;
};

}   // namespace hiper

#endif   // HIPER_FIBER_H
