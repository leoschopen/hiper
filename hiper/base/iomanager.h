/*
 * @Author: Leo
 * @Date: 2023-08-29 20:30:37
 * @LastEditTime: 2023-08-31 20:21:51
 * @Description: IO协程调度器
 */

#ifndef HIPER_IOMANAGER_H
#define HIPER_IOMANAGER_H

#include "scheduler.h"
#include "timer.h"

namespace hiper {

class IOManager : public Scheduler, public TimerManger {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex                    RWMutexType;

    enum Event
    {
        NONE  = 0x0,   // 无事件
        READ  = 0x1,   // 读事件
        WRITE = 0x4,   // 写事件
    };

private:
    /**
     * @brief socket fd上下文类
     * 
     */
    struct FdContext
    {
        typedef Mutex MutexType;

        struct EventContext
        {
            Scheduler*            scheduler = nullptr;   // 事件执行的scheduler
            Fiber::ptr            fiber;                 // 事件协程
            std::function<void()> cb;                    // 事件回调函数
        };

        /**
         * @brief 返回事件的上下文
         * 
         * @param event 
         * @return EventContext& 
         */
        EventContext& getContext(Event event);

        void resetContext(EventContext& ctx);

        void triggerEvent(Event event);

        EventContext read_context;            // 读事件
        EventContext write_context;           // 写事件
        int          fd     = 0;      // 事件关联的文件描述符
        Event        events = NONE;   // 已经注册的事件
        MutexType    mutex;           // 事件的互斥量
    };

public:
    /**
     * @brief IO事件协程调度器构造函数，支持epoll，重载tickle和idle
     * 
     * @param threads 
     * @param use_caller 
     * @param name 
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    ~IOManager();

    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    /**
     * @brief 删除事件，重置该fd的上下文：包括监控的事件和清空事件上下文，防止事件被触发
     * @note  epoll返回前调用，防止事件被触发
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief 取消事件，取消监控事件，触发该事件
     * @note  用于epoll返回之后，取消事件，由于os已经检测到事件，所以需要手动触发后再删除
     */
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:

    // 通知调度协程
    void tickle() override;

    bool stopping() override;

    /**
    * @brief idle协程
    * @details 对于IO协程调度来说，应阻塞在等待IO事件上，idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件就绪
    * 调度器无调度任务时会阻塞idle协程上，对IO调度器而言，idle状态应该关注两件事，一是有没有新的调度任务，对应Schduler::schedule()，
    * 如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务；二是关注当前注册的所有IO事件有没有触发，如果有触发，那么应该执行
    * IO事件对应的回调函数
    */
    void idle() override;

    void onTimerInsertedAtFront() override;

    /**
     * @brief 重置socket句柄上下文的容器大小
     * @param[in] size 容量大小
     */
    void contextResize(size_t size);

    /**
     * @brief 判断是否可以停止
     * @param[out] timeout 最近要触发的定时器事件间隔
     * @return 返回是否可以停止
     */
    bool stopping(uint64_t& timeout);

private:
    int epoll_fd_ = 0;
    // pipe 文件句柄
    int tickle_fds_[2];
    // 当前等待执行的事件数量
    std::atomic<size_t> pending_event_count_ = {0};

    RWMutexType mutex_;
    /**
     * @brief socket事件上下文的容器,依次存放epoll_fd监控的socket事件
     * 
     */
    std::vector<FdContext*> fd_contexts_;
};


}   // namespace hiper

#endif   // HIPER_IOMANAGER_H