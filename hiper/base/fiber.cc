/*
 * @Author: Leo
 * @Date: 2023-08-09 15:33:13
 * @LastEditTime: 2023-08-28 20:55:42
 * @Description: fiber implementation
 */

#include "fiber.h"

#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mimalloc-2.1/mimalloc.h>
#include <sys/types.h>
#include <ucontext.h>

namespace hiper {

static Logger::ptr g_logger = LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

// 正在执行的协程
static thread_local Fiber* t_fiber = nullptr;

// 当前线程的主协程，提供一个切换的起点，切换该协程相当于切换到了主线程中运行
static thread_local Fiber::ptr t_thread_main_fiber = nullptr;

static thread_local std::string t_fiber_name = "main";
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");


class MallocStackAllocator {
public:
    static void* Alloc(size_t size) { return mi_malloc(size); }

    static void Dealloc(void* vp, size_t size) { mi_free(vp); }
};


using StackAllocator = MallocStackAllocator;   // 方便后续替换



/**
 * @brief 创建主协程,利用线程的上下文初始化主协程。
 * @note 作为第一次切换的起点协程。保持主协程不断的可执行,以便随时切换回主协程执行。
 */
Fiber::Fiber()
{
    state_ = EXEC;
    SetThis(this);
    if (getcontext(&ctx_)) {
        HIPER_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;
    LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}



/**
 * @brief 有回调函数的协程，能够创建协程
 *
 * @param cb
 * @param stacksize
 * @param back_to_caller
 */
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool back_to_caller)
    : id_(++s_fiber_id)
    , cb_(cb)
    , back_to_caller_(back_to_caller)
{
    ++s_fiber_count;
    stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();

    stack_ = StackAllocator::Alloc(stacksize_);
    if (getcontext(&ctx_)) {
        HIPER_ASSERT2(false, "getcontext");
    }

    // 初始化上下文m_ctx,将协程栈stack_关联到上下文
    ctx_.uc_link          = nullptr;
    ctx_.uc_stack.ss_sp   = stack_;
    ctx_.uc_stack.ss_size = stacksize_;


    makecontext(&ctx_, &Fiber::MainFunc, 0);

    LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << id_;
}


Fiber::~Fiber()
{
    --s_fiber_count;
    if (stack_) {
        HIPER_ASSERT(state_ == TERM || state_ == EXCEPT || state_ == INIT);
        StackAllocator::Dealloc(stack_, stacksize_);
    }
    else {
        // 无栈，说明是主协程
        HIPER_ASSERT(!cb_);
        HIPER_ASSERT(state_ == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << id_ << " total = " << s_fiber_count;
}

// 重置协程函数，并重置状态，利用已分配的内存继续使用
void Fiber::reset(std::function<void()> cb)
{
    HIPER_ASSERT(stack_);
    HIPER_ASSERT(state_ == TERM || state_ == EXCEPT || state_ == INIT);
    cb_ = cb;

    if (getcontext(&ctx_)) {
        HIPER_ASSERT2(false, "getcontext");
    }

    ctx_.uc_link          = nullptr;
    ctx_.uc_stack.ss_sp   = stack_;
    ctx_.uc_stack.ss_size = stacksize_;

    makecontext(&ctx_, &Fiber::MainFunc, 0);
    state_ = INIT;
}

// 切换到当前协程执行
void Fiber::resume()
{
    SetThis(this);
    HIPER_ASSERT(state_ != EXEC);
    state_ = EXEC;
    if (back_to_caller_) {
        // LOG_DEBUG(g_logger) << " back_to_caller_ from " << t_thread_main_fiber->id_;
        if (swapcontext(&t_thread_main_fiber->ctx_, &ctx_)) {
            HIPER_ASSERT2(false, "swapcontext");
        }
    }
    else {
        // LOG_INFO(g_logger) << " not back_to_caller_ from "  << Scheduler::GetSchedulerFiber()->id_;
        if (swapcontext(&Scheduler::GetSchedulerFiber()->ctx_, &ctx_)) {
            HIPER_ASSERT2(false, "swapcontext");
        }
    }
}

// 将当前协程切换到后台
void Fiber::yield()
{
    if (back_to_caller_) {
        // LOG_DEBUG(g_logger) << " back_to_caller_ yield to " << t_thread_main_fiber->id_;
        SetThis(t_thread_main_fiber.get());
        if (swapcontext(&ctx_, &t_thread_main_fiber->ctx_)) {
            HIPER_ASSERT2(false, "swapcontext");
        }
    }
    else {
        // LOG_INFO(g_logger) << " not back_to_caller_ yield to "  << Scheduler::GetSchedulerFiber()->id_;
        SetThis(Scheduler::GetSchedulerFiber());
        if (swapcontext(&ctx_, &Scheduler::GetSchedulerFiber()->ctx_)) {
            HIPER_ASSERT2(false, "swapcontext");
        }
    }
}

void Fiber::SetThis(Fiber* f)
{
    // LOG_DEBUG(g_logger) << "SetThis from " << t_fiber << " to " << f->getId();
    
    t_fiber = f;
}

// 返回当前协程
Fiber::ptr Fiber::GetThis()
{
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    LOG_INFO(g_logger) << "create main fiber";
    Fiber::ptr main_fiber(new Fiber);
    HIPER_ASSERT(t_fiber == main_fiber.get());
    t_thread_main_fiber = main_fiber;
    return t_fiber->shared_from_this();
}

// 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady()
{
    Fiber::ptr cur = GetThis();
    HIPER_ASSERT(cur->state_ == EXEC);
    cur->state_ = READY;
    cur->yield();
}

// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    HIPER_ASSERT(cur->state_ == EXEC);
    cur->state_ = HOLD;
    cur->yield();
}

// 总协程数
uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

void Fiber::MainFunc()
{
    Fiber::ptr cur = GetThis();
    HIPER_ASSERT(cur);
    try {
        cur->cb_();
        cur->cb_    = nullptr;
        cur->state_ = TERM;
    }
    catch (std::exception& ex) {
        cur->state_ = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
                            << std::endl
                            << hiper::BacktraceToString();
    }
    catch (...) {
        cur->state_ = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber Except"
                            << " fiber_id=" << cur->getId() << std::endl
                            << hiper::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->yield();

    HIPER_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

uint64_t Fiber::GetFiberId()
{
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

}   // namespace hiper