/*
 * @Author: Leo
 * @Date: 2023-08-24 10:54:21
 * @LastEditTime: 2023-08-24 17:48:44
 * @Description: 协程调度
 */

#ifndef HIPER_SCHEDULER_H
#define HIPER_SCHEDULER_H

#include "fiber.h"
#include "mutex.h"
#include "noncopyable.h"
#include "thread.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <ostream>
#include <vector>

namespace hiper {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex                      MutexType;

    /**
     * @brief Construct a new Scheduler object
     *
     * @param threads 线程池的大小，threads num
     * @param use_caller 线程是否纳入协程调度器中，作为主线程运行主协程
     * @param name 调度器的名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    virtual ~Scheduler();

    const std::string& getName() const { return name_; }

    static Scheduler* GetThis();
    static Fiber*     GetMainFiber();

    void start();
    void stop();

    // 协程调度函数
    template<class FiberOrCb> void schedule(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(mutex_);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if (need_tickle) {
            tickle();
        }
    }

    // 协程批量调度函数
    template<class InputIterator> void schedule(InputIterator begin, InputIterator end)
    {
        bool need_tickle = false;
        {
            MutexType::Lock lock(mutex_);
            while (begin != end) {
                // 或运算，防止被覆盖，只要有任何一次需要通知，就会正确通知调度线程
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }

    void switchTo(int thread = -1);

    std::ostream& dump(std::ostream& os);

protected:
    virtual void tickle();

    void run();

    virtual bool stopping();

    virtual void idle();   // 空闲时调度idle协程

    void setThis();

    bool hasIdleThreads() { return idle_thread_count_ > 0; }

private:
    struct FiberAndThread
    {
        Fiber::ptr            fiber;
        std::function<void()> cb;
        int                   thread_id;   // 指定协程在哪个线程上执行

        /**
         * @brief
         * 记录协程/函数要在哪个线程上执行,使调度器可以将任务调度到指定的线程上，方便调度器的实现。
         * 智能指针与智能指针的指针两种实现第一种共享相同的资源，计数增加，第二种在新的智能指针上管理，计数不变
         * 第一种是FiberAndThread
         * 对象共享一个智能指针，第二种是管理一个智能指针，提现了生命周期和资源的控制
         * @param f
         * @param thr
         */
        FiberAndThread(Fiber::ptr f, int thr)
            : fiber(f)
            , thread_id(thr)
        {}

        FiberAndThread(Fiber::ptr* f, int thr)
            : thread_id(thr)
        {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> c, int thr)
            : cb(c)
            , thread_id(thr)
        {}

        FiberAndThread(std::function<void()>* c, int thr)
            : thread_id(thr)
        {
            cb.swap(*c);
        }

        // STL容器盛放自定义类型需要的默认构造函数，否则无法初始化
        FiberAndThread()
            : thread_id(-1)
        {}

        void reset()
        {
            fiber     = nullptr;
            cb        = nullptr;
            thread_id = -1;
        }
    };

private:
    // 协程无锁调度函数，被schedule函数内部调用
    template<class FiberOrCb> bool scheduleNoLock(FiberOrCb fc, int thread)
    {
        bool need_tickle = fibers_.empty();

        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            fibers_.push_back(ft);
        }

        return need_tickle;
    }

private:
    MutexType                 mutex_;
    std::vector<Thread::ptr>  threads_;
    std::list<FiberAndThread> fibers_;
    Fiber::ptr                root_fiber_;   // use_caller为true时有效,调度协程
    std::string               name_;        // 调度器名称

protected:
    std::vector<int>    thread_ids_;
    size_t              thread_count_;
    std::atomic<size_t> active_thread_count_ = {0};
    std::atomic<size_t> idle_thread_count_   = {0};
    bool                stopping_            = true;
    bool                auto_stop_           = false;
    int                 root_thread_id_         = 0;   // 主线程id
};

class SchedulerSwitcher : public Noncopyable {
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();

private:
    Scheduler* caller_;
};

}   // namespace hiper

#endif   // HIPER_SCHEDULER_H
