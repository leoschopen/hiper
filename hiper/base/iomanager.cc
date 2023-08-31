#include "iomanager.h"

#include "hiper.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace hiper {

static hiper::Logger::ptr g_logger = LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event)
{
    switch (event) {
    case IOManager::READ: return read_context;
    case IOManager::WRITE: return write_context;
    default:
        LOG_ERROR(g_logger) << "getContext invalid event";
        throw std::invalid_argument("getContext invalid event");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx)
{
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
    HIPER_ASSERT(events & event);        // 事件应该已经包含在events中
    events = (Event)(events & ~event);   // 从events中删除该事件

    EventContext& ctx = getContext(event);
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    }
    else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
}



IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name)
{
    epoll_fd_ = epoll_create(5000);
    HIPER_ASSERT(epoll_fd_ > 0);

    int ret = pipe(tickle_fds_);
    HIPER_ASSERT(!ret);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events  = EPOLLIN | EPOLLET;
    event.data.fd = tickle_fds_[0];
    ret           = fcntl(tickle_fds_[0], F_SETFL, O_NONBLOCK);
    HIPER_ASSERT(!ret);

    ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, tickle_fds_[0], &event);
    HIPER_ASSERT(!ret);

    contextResize(32);
    start();
}


IOManager::~IOManager()
{
    stop();
    close(epoll_fd_);
    close(tickle_fds_[0]);
    close(tickle_fds_[1]);

    for (size_t i = 0; i < fd_contexts_.size(); ++i) {
        if (fd_contexts_[i]) {
            delete fd_contexts_[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
    // 找到fd对应的FdContext，如果不存在，那就分配一个
    FdContext*            fd_ctx = nullptr;
    RWMutexType::ReadLock lock(mutex_);
    if ((int)fd_contexts_.size() > fd) {
        fd_ctx = fd_contexts_[fd];
        lock.unlock();
    }
    else {
        lock.unlock();
        RWMutexType::WriteLock lock2(mutex_);
        fd_contexts_.resize(fd * 1.5);
        fd_ctx = fd_contexts_[fd];
    }

    // 同一个fd不允许重复添加相同的事件
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (HIPER_UNLIKELY(fd_ctx->events & event)) {
        LOG_ERROR(g_logger) << "addEvent assert fd = " << fd << " event = " << (EPOLL_EVENTS)event
                            << " fd_ctx.event = " << (EPOLL_EVENTS)fd_ctx->events;
        HIPER_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    epoll_event epevent;
    epevent.events   = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(epoll_fd_, op, fd, &epevent);
    if (ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << epoll_fd_ << ", " << op << ", " << fd << ", "
                            << (EPOLL_EVENTS)epevent.events << "): " << ret << " (" << errno
                            << ") (" << strerror(errno)
                            << ") fd_ctx->events=" << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    ++pending_event_count_;
    fd_ctx->events                     = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    HIPER_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    }
    else {
        event_ctx.fiber = Fiber::GetThis();
        HIPER_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                      "state = " << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
    // 界限检查
    RWMutexType::ReadLock lock(mutex_);
    if ((int)fd_contexts_.size() <= fd) {
        return false;
    }

    FdContext* fd_ctx = fd_contexts_[fd];
    lock.unlock();

    // 删除的事件要是已经存在的
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (HIPER_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 删除对应的事件，如果没有事件了，那就删除对应的fd，停止监听
    Event new_events = (Event)(fd_ctx->events & ~event);

    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events   = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(epoll_fd_, op, fd, &epevent);
    if (ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << epoll_fd_ << ", " << op << ", " << fd << ", "
                            << (EPOLL_EVENTS)epevent.events << "): " << ret << " (" << errno
                            << ") (" << strerror(errno)
                            << ") fd_ctx->events=" << (EPOLL_EVENTS)fd_ctx->events;
        return false;
    }

    // 修改该fd的上下文信息
    --pending_event_count_;

    fd_ctx->events = new_events;

    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event)
{
    // 界限检查
    RWMutexType::ReadLock lock(mutex_);
    if ((int)fd_contexts_.size() <= fd) {
        return false;
    }

    FdContext* fd_ctx = fd_contexts_[fd];
    lock.unlock();

    // 删除的事件要是已经存在的
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (HIPER_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 删除对应的事件，如果没有事件了，那就删除对应的fd，停止监听
    Event new_events = (Event)(fd_ctx->events & ~event);

    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    epevent.events   = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(epoll_fd_, op, fd, &epevent);
    if (ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << epoll_fd_ << ", " << op << ", " << fd << ", "
                            << (EPOLL_EVENTS)epevent.events << "): " << ret << " (" << errno
                            << ") (" << strerror(errno)
                            << ") fd_ctx->events=" << (EPOLL_EVENTS)fd_ctx->events;
        return false;
    }

    // 修改该fd的上下文信息
    --pending_event_count_;

    fd_ctx->triggerEvent(event);
    return true;
}

bool IOManager::cancelAll(int fd)
{
    // 界限检查
    RWMutexType::ReadLock lock(mutex_);
    if ((int)fd_contexts_.size() <= fd) {
        return false;
    }

    FdContext* fd_ctx = fd_contexts_[fd];
    lock.unlock();

    // 删除的事件要是已经存在的
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }
    int op = EPOLL_CTL_DEL;

    // Linux 2.6.9 之后， event 此时可以为空，但在这之前，event 需要非空（但不起作用）
    epoll_event epevent;
    epevent.events   = 0;
    epevent.data.ptr = fd_ctx;

    int ret = epoll_ctl(epoll_fd_, op, fd, &epevent);
    if (ret) {
        LOG_ERROR(g_logger) << "epoll_ctl(" << epoll_fd_ << ", " << op << ", " << fd << ", "
                            << (EPOLL_EVENTS)epevent.events << "): " << ret << " (" << errno
                            << ") (" << strerror(errno)
                            << ") fd_ctx->events=" << (EPOLL_EVENTS)fd_ctx->events;
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --pending_event_count_;
    }

    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --pending_event_count_;
    }

    HIPER_ASSERT(fd_ctx->events == NONE);

    return true;
}

static IOManager* GetThis()
{
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


void IOManager::tickle()
{
    if (!hasIdleThreads()) {
        return;
    }
    int ret = write(tickle_fds_[1], "T", 1);
    HIPER_ASSERT(ret == 1);
}

bool IOManager::stopping(uint64_t& timeout)
{
    timeout = getNextTimer();
    return timeout == ~0ull && pending_event_count_ == 0 && Scheduler::stopping();
}

void IOManager::idle()
{
    LOG_DEBUG(g_logger) << "idle";
    
    // 一次epoll_wait最多检测256个就绪事件，如果就绪事件超过了这个数，那么会在下轮epoll_wati继续处理
    const uint64_t MAX_EVNETS = 256;

    epoll_event* events = new epoll_event[MAX_EVNETS]();

    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) { delete[] ptr; });

    while (true) {
        uint64_t next_timeout = 0;
        if (HIPER_UNLIKELY(stopping(next_timeout))) {
            LOG_INFO(g_logger) << "name = " << getName() << " idle stopping exit";
            break;
        }

        // 阻塞在epoll_wait上，等待事件发生或定时器超时
        int ret = 0;
        do {
            static const int MAX_TIMEOUT = 5000;
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            }
            else {
                next_timeout = MAX_TIMEOUT;
            }
            ret = epoll_wait(epoll_fd_, events, MAX_EVNETS, (int)next_timeout);
            if (ret < 0 && errno == EINTR) {
                continue;
            }
            else {
                break;
            }
        } while (true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if (!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        // 遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，进行事件处理
        for (int i = 0; i < ret; ++i) {
            epoll_event& event = events[i];
            if (event.data.fd == tickle_fds_[0]) {
                uint8_t dummy[256];
                while (read(tickle_fds_[0], dummy, 256) > 0)
                    ;
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr;

            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            /**
             * EPOLLERR: 出错，比如写读端已经关闭的pipe
             * EPOLLHUP: 套接字对端关闭
             * 出现这两种事件，应该同时触发fd的读和写事件，否则有可能出现注册的事件永远执行不到的情况
             */
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op          = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

            event.events = EPOLLET | left_events;

            int ret2 = epoll_ctl(epoll_fd_, op, fd_ctx->fd, &event);
            if (ret2) {
                LOG_ERROR(g_logger) << "epoll_ctl(" << epoll_fd_ << ", " << op << ", " << fd_ctx->fd
                                    << ", " << (EPOLL_EVENTS)event.events << "):" << ret2 << " ("
                                    << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --pending_event_count_;
            }

            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --pending_event_count_;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();

        auto raw_ptr = cur.get();

        cur.reset();
        raw_ptr->yield();
    }
}


void IOManager::onTimerInsertedAtFront()
{
    tickle();
}


void IOManager::contextResize(size_t size)
{
    fd_contexts_.resize(size);
    for (size_t i = 0; i < fd_contexts_.size(); ++i) {
        if (!fd_contexts_[i]) {
            fd_contexts_[i]     = new FdContext;
            fd_contexts_[i]->fd = i;
        }
    }
}


}   // namespace hiper