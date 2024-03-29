#include "scheduler.h"

#include "log.h"
#include "macro.h"

namespace hiper {

static hiper::Logger::ptr g_logger = LOG_NAME("system");

// 当前线程的调度器对象
static thread_local Scheduler* t_scheduler = nullptr;

// 当前线程的调度协程，也是主协程
static thread_local Fiber* t_scheduler_fiber = nullptr;


/**
 * @brief Construct a new Scheduler:: Scheduler object
 *
 * @param size
 * @param use_cur_thread 控制是否使用调用者线程来执行调度器还是创建一个独立的线程来执行调度逻辑
 * @param name
 */
Scheduler::Scheduler(size_t size, bool use_cur_thread, const std::string& name)
    : name_(name)
{
    HIPER_ASSERT(size > 0);

    if (use_cur_thread) {
        hiper::Fiber::GetThis();

        // threads只表示额外创建的线程池大小，主协程已经占用了一个线程
        --size;

        // 确保没有其他调度器存在
        HIPER_ASSERT(GetThis() == nullptr);

        t_scheduler = this;

        caller_scheduler_fiber_.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));

        hiper::Thread::SetName(name_);

        t_scheduler_fiber           = caller_scheduler_fiber_.get();
        caller_scheduler_thread_id_ = hiper::GetThreadId();
        thread_ids_.push_back(caller_scheduler_thread_id_);
    }
    else {
        caller_scheduler_thread_id_ = -1;
    }
    thread_count_ = size;
}

Scheduler::~Scheduler()
{
    HIPER_ASSERT(stopping_);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

/**
 * @brief 获取当前线程的调度器对象
 *
 * @return Scheduler*
 */
Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}


/**
 * @brief 返回调度器的调度协程
 *
 * @return Fiber*
 */
Fiber* Scheduler::GetSchedulerFiber()
{
    return t_scheduler_fiber;
}


// 根据线程数创建若干线程，每个线程将会运行着一个调度器
void Scheduler::start()
{
    MutexType::Lock lock(mutex_);
    if (!stopping_) {
        return;
    }
    stopping_ = false;
    HIPER_ASSERT(threads_.empty());

    threads_.resize(thread_count_);
    for (size_t i = 0; i < thread_count_; ++i) {
        threads_[i].reset(
            new Thread(std::bind(&Scheduler::run, this), name_ + "_" + std::to_string(i)));
        thread_ids_.push_back(threads_[i]->getId());
    }
    lock.unlock();
}

void Scheduler::stop()
{
    auto_stop_ = true;   // 调度器将会执行自动停止操作

    // 关闭caller线程中的调度器
    if (caller_scheduler_fiber_ && thread_count_ == 0 &&
        (caller_scheduler_fiber_->getState() == Fiber::TERM ||
         caller_scheduler_fiber_->getState() == Fiber::INIT)) {
        LOG_INFO(g_logger) << this << " stopped";
        stopping_ = true;

        // 如果调度器正在停止中，直接返回
        if (stopping()) {
            return;
        }
    }

    // caller_scheduler_thread_id_ 标识调度器是否是在调用者线程中执行还是创建的额外的线程中执行


    // 调度器如果是在caller线程中创建的话，只有caller线程才能stop
    // 调度器在额外线程中创建，在caller线程中stop，getThis()返回的是nullptr
    if (caller_scheduler_thread_id_ != -1) {
        HIPER_ASSERT(GetThis() == this);
    }
    else {
        HIPER_ASSERT(GetThis() != this);
    }

    stopping_ = true;
    for (size_t i = 0; i < thread_count_; ++i) {
        tickle();
    }

    if (caller_scheduler_fiber_) {
        tickle();
    }

    if (caller_scheduler_fiber_) {
        if (!stopping()) {
            caller_scheduler_fiber_->resume();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(mutex_);
        thrs.swap(threads_);
    }

    for (auto& i : thrs) {
        i->join();
    }
}

void Scheduler::setThis()
{
    t_scheduler = this;
}

/**
 * @brief
 * 调度器的核心执行函数。它会在调度器的主线程（或调用者线程）中不断循环执行，选择并执行就绪的协程。
 *
 */
void Scheduler::run()
{
    LOG_DEBUG(g_logger) << " " << name_ << " run";
    // set_hook_enable(true);

    // 设置线程内的局部调度器变量,表示这个Scheduler开始负责当前线程上的调度工作。
    setThis();

    // 如果是在调用者线程中执行，那么t_scheduler_fiber被设置为root_fiber_
    // 否则在这里设置为本线程内的主协程
    if (hiper::GetThreadId() != caller_scheduler_thread_id_) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        // LOG_DEBUG(g_logger) << "Scheduler Run";
        ft.reset();
        bool tickle_me = false;   // 标记是否需要通知其他线程进行调度
        bool is_active = false;   // 标记是否有协程被调度
        {
            MutexType::Lock lock(mutex_);
            auto            it = fibers_.begin();

            while (it != fibers_.end()) {

                // 如果一个任务已经指定了运行的线程，并且指定的这个线程不是当前线程，那么跳过这个任务
                if (it->thread_id != -1 && it->thread_id != hiper::GetThreadId()) {
                    ++it;
                    tickle_me = true;   // 虽然本线程不处理，但是要通知其他线程进行调度
                    continue;
                }

                HIPER_ASSERT(it->fiber || it->cb);

                // 当前fiber处于执行状态，状态不可调度，跳过
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                fibers_.erase(it++);
                ++active_thread_count_;
                is_active = true;
                break;
            }
            // 判断是否需要通知其他线程进行调度
            tickle_me |= it != fibers_.end();
        }

        if (tickle_me) {
            tickle();
        }

        if (ft.fiber &&
            (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->resume();
            --active_thread_count_;

            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);   // 将协程放回调度队列
            }

            // 执行调度后，协程状态不是TERM或EXCEPT，说明协程还没有执行完，需要继续执行
            else if (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->state_ = Fiber::HOLD;
            }

            ft.reset();
        }

        else if (ft.cb) {
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            }
            else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->resume();
            --active_thread_count_;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            }
            else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            }
            else {   // if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->state_ = Fiber::HOLD;
                cb_fiber.reset();
            }
        }
        else {
            
            if (is_active) {
                --active_thread_count_;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                LOG_INFO(g_logger) << " idle fiber (" << idle_fiber->getId() <<") term";
                break;
            }
            ++idle_thread_count_;
            idle_fiber->resume();
            --idle_thread_count_;
            if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->state_ = Fiber::HOLD;
            }
        }
    }
}

/**
 * @brief 类似信号量，tickle之后可以唤醒线程，让它们有机会检查 stopping_ 标志并退出循环。
 *
 */
void Scheduler::tickle()
{
    LOG_INFO(g_logger) << " tickle";
}

bool Scheduler::stopping()
{
    MutexType::Lock lock(mutex_);
    return auto_stop_ && stopping_ && fibers_.empty() && active_thread_count_ == 0;
}

void Scheduler::idle()
{
    LOG_INFO(g_logger) << " idle";
    while (!stopping()) {
        // LOG_INFO(g_logger) << " idle yield to hold";
        hiper::Fiber::YieldToHold();
    }
}

/**
 * @brief 切换当前协程到另一个线程中执行
 *
 * @param thread
 */
void Scheduler::switchTo(int thread)
{
    HIPER_ASSERT(Scheduler::GetThis() != nullptr);

    // 如果不需要指定线程或者目标线程就是本线程
    if (Scheduler::GetThis() == this) {
        if (thread == -1 || thread == hiper::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    // 让出执行权并暂停，使得目标线程调度
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os)
{
    os << "[Scheduler name=" << name_ << " size=" << thread_count_
       << " active_count=" << active_thread_count_ << " idle_count=" << idle_thread_count_
       << " stopping=" << stopping_ << " ]" << std::endl
       << "    ";
    for (size_t i = 0; i < thread_ids_.size(); ++i) {
        if (i) {
            os << ", ";
        }
        os << thread_ids_[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target)
{
    caller_ = Scheduler::GetThis();
    if (target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher()
{
    if (caller_) {
        caller_->switchTo();
    }
}

}   // namespace hiper