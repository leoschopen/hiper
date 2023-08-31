#include "timer.h"

#include "util.h"

#include <cstdint>
#include <sys/types.h>


Timer::Timer(uint64_t ms, TimeoutCallBack cb, bool is_recurring, TimerManger* manager)
{
    is_recurring_  = is_recurring;
    ms_            = ms;
    cb_            = cb;
    timer_manager_ = manager;
    expiration_    = hiper::GetElapsedMS() + ms;
}

Timer::Timer(uint64_t expiration)
{
    expiration_ = expiration;
}

/**
 * @brief 取消定时器，将定时器从定时器管理队列中删除
 *
 * @return true
 * @return false
 */
bool Timer::cancel()
{
    TimerManger::RWMutexType::WriteLock lock(timer_manager_->mtx_);
    if (cb_) {
        cb_      = nullptr;
        auto ret = timer_manager_->timers_.find(shared_from_this());
        if (ret != timer_manager_->timers_.end()) {
            timer_manager_->timers_.erase(ret);
            return true;
        }
    }
    return false;
}

/**
 * @brief 刷新某个定时器，将其时间设置为从现在开始
 *
 * @return true
 * @return false
 */
bool Timer::refresh()
{
    TimerManger::RWMutexType::WriteLock lock(timer_manager_->mtx_);
    if (!cb_) {
        return false;
    }
    auto ret = timer_manager_->timers_.find(shared_from_this());
    if (ret == timer_manager_->timers_.end()) {
        return false;
    }
    timer_manager_->timers_.erase(ret);
    expiration_ = hiper::GetElapsedMS() + ms_;
    timer_manager_->timers_.insert(shared_from_this());
    return true;
}


/**
 * @brief 重置定时器，将定时器的执行时间设置为ms
 *
 * @param ms
 * @param from_now
 * @return true
 * @return false
 */
bool Timer::reset(uint64_t ms, bool from_now)
{
    if (ms == ms_ && !from_now) {
        return true;
    }
    TimerManger::RWMutexType::WriteLock lock(timer_manager_->mtx_);
    if (!cb_) {
        return false;
    }
    auto ret = timer_manager_->timers_.find(shared_from_this());
    if (ret == timer_manager_->timers_.end()) {
        return false;
    }
    timer_manager_->timers_.erase(ret);

    uint64_t start = 0;
    if (from_now) {
        start = hiper::GetElapsedMS();
    }
    else {
        start = expiration_ - ms_;
    }
    ms_         = ms;
    expiration_ = start + ms_;

    // TODO: 是否需要传入参数lock
    timer_manager_->addTimer(shared_from_this(), lock);
    return true;
}


Timer::ptr TimerManger::addTimer(uint64_t ms, const TimeoutCallBack& cb, bool recurring)
{
    Timer::ptr timer(new Timer(ms, cb, recurring, this));

    RWMutexType::WriteLock lock(mtx_);
    addTimer(timer, lock);
    return timer;
}


/**
 * @brief 添加定时器,并判断定时器是否插入了最前面从而判断是否触发函数
 * @note  主要是因为添加在最前面的定时器需要通知其他线程进行调度
 *
 * @param timer
 * @param lock
 */
void TimerManger::addTimer(const Timer::ptr& timer, RWMutexType::WriteLock& lock)
{
    auto it       = timers_.insert(timer).first;
    bool at_front = (it == timers_.begin()) && !tickled_;
    if (at_front) {
        tickled_ = true;
    }
    lock.unlock();
    if (at_front) {
        onTimerInsertedAtFront();
    }
}

/**
 * @brief 在定时器触发时，检查弱指针是否有效，如果有效，则调用回调函数
 *
 * @param weak_cond
 * @param cb
 */
static void OnTimer(const std::weak_ptr<void>& weak_cond, std::function<void()> cb)
{
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

/**
 * @brief 添加一个条件定时器,该定时器的触发受一个弱指针的生命周期控制。
 * @note
 * 通过一个外部对象的生命周期来控制定时器是否继续工作,避免定时器引用了已销毁的对象导致undefined
 * behavior。
 *
 * @param ms
 * @param cb
 * @param weak_cond
 * @param recurring
 * @return Timer::ptr
 */
Timer::ptr TimerManger::addConditionTimer(uint64_t ms, const TimeoutCallBack& cb,
                                          const std::weak_ptr<void>& weak_cond, bool recurring)
{
    // return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
    return addTimer(
        ms, [weak_cond, cb] { return OnTimer(weak_cond, cb); }, recurring);
}



uint64_t TimerManger::getNextTimer()
{
    RWMutexType::WriteLock lock(mtx_);
    tickled_ = false;
    if (timers_.empty()) {
        return ~0ull;
    }
    const Timer::ptr& next = *timers_.begin();
    uint64_t          now  = hiper::GetElapsedMS();
    if (now >= next->expiration_) {
        return 0;
    }
    else {
        return next->expiration_ - now;
    }
}


void TimerManger::listExpiredCb(std::vector<std::function<void()>>& cbs)
{
    uint64_t                now = hiper::GetElapsedMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::WriteLock lock(mtx_);
        if (timers_.empty()) {
            return;
        }
    }

    RWMutexType::WriteLock lock(mtx_);
    if (timers_.empty()) {
        return;
    }

    // bool rollover = detectClockRollover(now);

    // if (!rollover && (*timers_.begin())->expiration_ > now) {
    //     return;
    // }

    Timer::ptr now_timer(new Timer(now));

    // 找到第一个大于等于目标值的元素，也就是第一个过期的定时器
    auto it = timers_.lower_bound(now_timer);

    while (it != timers_.end() && (*it)->expiration_ == now) {
        ++it;
    }

    expired.insert(expired.begin(), timers_.begin(), it);
    timers_.erase(timers_.begin(), it);
    cbs.reserve(expired.size());
    for (auto& timer : expired) {
        cbs.push_back(timer->cb_);
        if (timer->is_recurring_) {
            timer->expiration_ = now + timer->ms_;
            timers_.insert(timer);
        }
        else {
            timer->cb_ = nullptr;
        }
    }
}

bool TimerManger::hasTimer()
{
    RWMutexType::WriteLock lock(mtx_);
    return !timers_.empty();
}

// bool TimerManger::detectClockRollover(uint64_t now)
// {
//     bool rollover = false;
//     if (now < previouse_time_ && now < (previouse_time_ - MS(60 * 60 * 1000 * 2))) {
//         rollover = true;
//     }
//     previouse_time_ = now;
//     return rollover;
// }


bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    // 升序
    if (lhs->expiration_ < rhs->expiration_) {
        return true;
    }
    if (rhs->expiration_ < lhs->expiration_) {
        return false;
    }
    // 如果两个定时器触发时间相同，则比较它们在内存中的地址，以保证排序的稳定性。
    return lhs.get() < rhs.get();
}
