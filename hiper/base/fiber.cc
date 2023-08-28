/*
 * @Author: Leo
 * @Date: 2023-08-09 15:33:13
 * @LastEditTime: 2023-08-28 12:32:55
 * @Description: fiber implementation
 */

#include "fiber.h"

#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

#include <cstdio>
#include <mimalloc-2.1/mimalloc.h>
// #include <mimalloc-2.1/mimalloc-new-delete.h>
#include "config.h"
// #include "mimalloc/mimalloc.h"

#include <atomic>
#include <cstdint>
#include <sys/types.h>
#include <ucontext.h>

namespace hiper {

static Logger::ptr g_logger = LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber*     t_fiber       = nullptr;
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

/*
    创建主协程,利用线程的上下文初始化主协程。
    作为第一次切换的起点协程。
    保持主协程不断的可执行,以便随时切换回主协程执行。
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
 * @param use_caller
 */
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : id_(++s_fiber_id)
    , cb_(cb)
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

    if (!use_caller) {
        // 创建协程，初始化上下文ctx,设置协程执行函数
        makecontext(&ctx_, &Fiber::MainFunc, 0);
    }
    else {
        makecontext(&ctx_, &Fiber::CallerMainFunc, 0);
    }

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
void Fiber::swapIn()
{
    SetThis(this);
    HIPER_ASSERT(state_ != EXEC);
    state_ = EXEC;
    if (swapcontext(&Scheduler::GetMainFiber()->ctx_, &ctx_)) {
        HIPER_ASSERT2(false, "swapcontext");
    }
    // if (swapcontext(&t_thread_main_fiber->ctx_, &ctx_)) {
    //     HIPER_ASSERT2(false, "swapcontext");
    // }
}

// 将当前协程切换到后台
void Fiber::swapOut()
{
    SetThis(t_thread_main_fiber.get());
    // if (swapcontext(&ctx_, &t_thread_main_fiber->ctx_)) {
    //     HIPER_ASSERT2(false, "swapcontext");
    // }
    if(swapcontext(&ctx_, &Scheduler::GetMainFiber()->ctx_)) {
        HIPER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::SetThis(Fiber* f)
{
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
    cur->swapOut();
}

// 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold()
{
    Fiber::ptr cur = GetThis();
    HIPER_ASSERT(cur->state_ == EXEC);
    cur->state_ = HOLD;
    cur->swapOut();
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

    // LOG_DEBUG(g_logger) << cur.use_count();  // 2

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    HIPER_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::call()
{
    SetThis(this);
    state_ = EXEC;
    if (swapcontext(&t_thread_main_fiber->ctx_, &ctx_)) {
        HIPER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back()
{
    SetThis(t_thread_main_fiber.get());
    if (swapcontext(&ctx_, &t_thread_main_fiber->ctx_)) {
        HIPER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::CallerMainFunc()
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
    raw_ptr->back();
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