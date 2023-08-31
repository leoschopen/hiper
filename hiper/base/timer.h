/*
 * @Author: Leo
 * @Date: 2023-08-29 20:33:43
 * @LastEditTime: 2023-08-30 22:00:05
 * @Description: 基于epoll超时实现定时器功能，精度毫秒级，支持在指定超时时间结束之后执行回调函数。
 */
#ifndef HIPER_TIMER_H
#define HIPER_TIMER_H


#include "mutex.h"

#include <chrono>
#include <functional> 
#include <memory>
#include <netinet/in.h>
#include <set>

class TimerManger;

typedef std::function<void()> TimeoutCallBack;


class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManger;

public:
    typedef std::shared_ptr<Timer> ptr;

    bool cancel();    // 取消定时器
    bool refresh();   // 刷新定时器的执行时间，改为当前时间+ms
    bool reset(uint64_t ms, bool from_now);   // 重置定时器时间

private:
    Timer(uint64_t ms, TimeoutCallBack cb, bool recurring, TimerManger* manager);
    Timer(uint64_t expires);

private:
    struct Comparator
    {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

private:
    bool            is_recurring_;              // 是否循环定时器
    uint64_t        ms_;                        // 相对执行时间
    uint64_t        expiration_;                // 绝对执行时间
    TimeoutCallBack cb_;                        // 回调函数
    TimerManger*    timer_manager_ = nullptr;   // 定时器管理器
};



class TimerManger {
    friend class Timer;

public:
    typedef hiper::RWMutex RWMutexType;

    TimerManger();

    virtual ~TimerManger() = default;

    Timer::ptr addTimer(uint64_t ms, const TimeoutCallBack& cb, bool recurring = false);

    Timer::ptr addConditionTimer(uint64_t ms, const std::function<void()>& cb,
                                 const std::weak_ptr<void>& weak_cond, bool recurring = false);
    /**
     * @brief 到最近一个定时器执行的时间间隔(毫秒)
     *
     * @return uint64_t
     */
    uint64_t getNextTimer();
    
    /**
     * @brief 获取需要执行的定时器的回调函数列表
     * 
     * @param cbs 存放回调函数的列表
     */
    void listExpiredCb(std::vector<std::function<void()>>& cbs);   // 获取需要执行的定时器回调函数

    bool hasTimer();   // 是否有定时器

protected:
    virtual void onTimerInsertedAtFront() = 0;   // 当有新的定时器插入到定时器首部时执行该函数

    void addTimer(const Timer::ptr& val, RWMutexType::WriteLock& lock);   // 添加定时器

// private:
//     /**
//      * @brief 检测服务器时间是否被调后了
//      */
//     bool detectClockRollover(uint64_t now); 

private:
    RWMutexType mtx_;

    std::set<Timer::ptr, Timer::Comparator> timers_;   // 定时器集合

    bool tickled_ = false;   // 是否触发onTimerInsertedAtFront
};

#endif   // HIPER_TIMER_H