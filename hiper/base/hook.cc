#include "config.h"
#include "hiper.h"

#include <dlfcn.h>

hiper::Logger::ptr g_logger = LOG_ROOT();

namespace hiper {

static hiper::ConfigVar<int>::ptr g_tcp_connect_timeout =
    hiper::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

// hook启用标志
static thread_local bool t_hook_enable = false;

static uint64_t s_connect_timeout = -1;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)



// 初始化原函数指针,由静态变量调用，在main运行时就会获得各个符号的地址
void hook_init()
{
    static bool is_inited = false;
    if (is_inited) {
        return;
    }

// 宏拼接操作符 ##
// 依次定义各个函数指针变量，并将其初始化为对应函数的指针。
// 在宏展开时，等号前的宏名称会被替换为等号后的替换文本
// 定义read_f = (read_func)dlsym(RTLD_NEXT, "read");
#define XX(name) name##_old = (name##_func)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}


struct _HookIniter
{
    _HookIniter()
    {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
            LOG_INFO(g_logger) << "tcp connect timeout changed from " << old_value << " to "
                               << new_value;
            s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;

bool is_hook_enable()
{
    return t_hook_enable;
}

void set_hook_enable(bool flag)
{
    t_hook_enable = flag;
}


}   // namespace hiper



struct timer_info
{
    int cancelled = 0;
};



/**
 * @brief 非阻塞 I/O 处理, 通过设置超时时间,将阻塞 I/O 转换为非阻塞 I/O
 *        在系统调用可能阻塞时自动执行一系列处理,将其变成异步执行,从而避免阻塞。配合协程调度,可以实现非阻塞IO模型。
 *
 * @tparam OriginFunc
 * @tparam Args
 * @param fd
 * @param func
 * @param hook_func_name
 * @param event Event 事件
 * @param timeout_so 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
 * @param args
 * @return ssize_t
 */
template<typename OriginFunc, typename... Args>
static ssize_t do_io(int fd, OriginFunc func, const char* hook_func_name, uint32_t event,
                     int timeout_so, Args&&... args)
{
    if (!hiper::t_hook_enable) {
        return func(fd, std::forward<Args>(args)...);
    }

    hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        return func(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    // 如果不是socket或者是非阻塞socket,直接调用原系统调用
    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return func(fd, std::forward<Args>(args)...);
    }

    uint64_t timeout = ctx->getTimeout(timeout_so);

    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = func(fd, std::forward<Args>(args)...);
    // 操作被中断,重新调用
    while (n == -1 && errno == EINTR) {
        n = func(fd, std::forward<Args>(args)...);
    }
    // 操作被阻塞，封装定时器，注册io事件，让出执行权
    if (n == -1 && errno == EAGAIN) {
        hiper::IOManager*         iom = hiper::IOManager::GetThis();
        hiper::Timer::ptr         timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if (timeout != (uint64_t)-1) {
            timer = iom->addConditionTimer(
                timeout,
                [winfo, fd, iom, event]() {
                    auto t = winfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (hiper::IOManager::Event)(event));
                },
                winfo);
        }

        int rt = iom->addEvent(fd, (hiper::IOManager::Event)(event));
        if (HIPER_UNLIKELY(rt)) {
            LOG_ERROR(g_logger) << hook_func_name << " addEvent(" << fd << ", " << event << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        }
        else {
            hiper::Fiber::GetThis()->yield();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }

    return n;
}





extern "C" {

#define XX(name) name##_func name##_old = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds)
{
    if (!hiper::t_hook_enable) {
        return sleep_old(seconds);
    }

    hiper::Fiber::ptr fiber = hiper::Fiber::GetThis();
    hiper::IOManager* iom   = hiper::IOManager::GetThis();
    // iom->addTimer(seconds * 1000,
    //               std::bind((void(hiper::Scheduler::*)(hiper::Fiber::ptr, int thread)) &
    //                             hiper::IOManager::schedule,
    //                         iom,
    //                         fiber,
    //                         -1));
    iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber, -1); });
    hiper::Fiber::GetThis()->yield();
    return 0;
}

int usleep(useconds_t usec)
{
    if (!hiper::t_hook_enable) {
        return usleep_old(usec);
    }
    hiper::Fiber::ptr fiber = hiper::Fiber::GetThis();
    hiper::IOManager* iom   = hiper::IOManager::GetThis();
    iom->addTimer(usec / 1000, [iom, fiber]() { iom->schedule(fiber, -1); });
    hiper::Fiber::GetThis()->yield();
    return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem)
{
    if (!hiper::t_hook_enable) {
        return nanosleep_old(req, rem);
    }

    int               timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    hiper::Fiber::ptr fiber      = hiper::Fiber::GetThis();
    hiper::IOManager* iom        = hiper::IOManager::GetThis();
    iom->addTimer(timeout_ms, [iom, fiber]() { iom->schedule(fiber, -1); });
    hiper::Fiber::GetThis()->yield();
    return 0;
}

// 使用socket获得到fd之后将其加入fdmanager的管理
int socket(int domain, int type, int protocol)
{
    if (!hiper::t_hook_enable) {
        return socket_old(domain, type, protocol);
    }
    int fd = socket_old(domain, type, protocol);
    // 如果fd
    if (fd == -1) {
        return fd;
    }
    hiper::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen,
                         uint64_t timeout_ms)
{
    if (!hiper::t_hook_enable) {
        return connect_old(fd, addr, addrlen);
    }
    hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket()) {
        return connect_old(fd, addr, addrlen);
    }

    if (ctx->getUserNonblock()) {
        return connect_old(fd, addr, addrlen);
    }

    int n = connect_old(fd, addr, addrlen);
    if (n == 0) {
        return 0;
    }
    else if (n != -1 || errno != EINPROGRESS) {
        return n;
    }

    hiper::IOManager*           iom = hiper::IOManager::GetThis();
    hiper::Timer::ptr           timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info>   winfo(tinfo);

    if (timeout_ms != (uint64_t)-1) {
        timer = iom->addConditionTimer(
            timeout_ms,
            [winfo, fd, iom]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, hiper::IOManager::WRITE);
            },
            winfo);
    }

    int rt = iom->addEvent(fd, hiper::IOManager::WRITE);
    if (rt == 0) {
        hiper::Fiber::GetThis()->yield();
        if (timer) {
            timer->cancel();
        }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }
    else {
        if (timer) {
            timer->cancel();
        }
        LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    // 检查连接结果
    int       error = 0;
    socklen_t len   = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    }
    else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    return connect_with_timeout(sockfd, addr, addrlen, hiper::s_connect_timeout);
}

int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
{
    int fd = do_io(s, accept_old, "accept", hiper::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        hiper::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void* buf, size_t count)
{
    return do_io(fd, read_old, "read", hiper::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
{
    return do_io(fd, readv_old, "readv", hiper::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags)
{
    return do_io(sockfd, recv_old, "recv", hiper::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr,
                 socklen_t* addrlen)
{
    return do_io(sockfd,
                 recvfrom_old,
                 "recvfrom",
                 hiper::IOManager::READ,
                 SO_RCVTIMEO,
                 buf,
                 len,
                 flags,
                 src_addr,
                 addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags)
{
    return do_io(sockfd, recvmsg_old, "recvmsg", hiper::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void* buf, size_t count)
{
    return do_io(fd, write_old, "write", hiper::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
{
    return do_io(fd, writev_old, "writev", hiper::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void* msg, size_t len, int flags)
{
    return do_io(s, send_old, "send", hiper::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to,
               socklen_t tolen)
{
    return do_io(
        s, sendto_old, "sendto", hiper::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr* msg, int flags)
{
    return do_io(s, sendmsg_old, "sendmsg", hiper::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd)
{
    if (!hiper::t_hook_enable) {
        return close_old(fd);
    }

    hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(fd);
    if (ctx) {
        auto iom = hiper::IOManager::GetThis();
        if (iom) {
            iom->cancelAll(fd);
        }
        hiper::FdMgr::GetInstance()->del(fd);
    }
    return close_old(fd);
}


// fcntl是用来修改已经打开文件的属性的函数
/*
 * fcntl function has these cmd arguments:
 * F_DUPFD、F_DUPFD_CLOEXEC : dup file descriptor
 * F_GETFD、F_SETFD : set file FD_CLOEXEC flag 当进程执行exec系统调用后此文件描述符会被自动关闭
 * F_GETFL、F_SETFL: file open flags 一般常用来设置做非阻塞读写操作
 * F_GETLK、F_SETLK、F_SETLKW : file lock 设置记录锁功能
 */
int fcntl(int fd, int cmd, ... /* arg */)
{
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
    // 设置文件状态标志
    case F_SETFL:
    {
        int arg = va_arg(va, int);
        va_end(va);
        hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return fcntl_old(fd, cmd, arg);
        }
        ctx->setUserNonblock(arg & O_NONBLOCK);
        if (ctx->getSysNonblock()) {
            arg |= O_NONBLOCK;
        }
        else {
            arg &= ~O_NONBLOCK;
        }
        return fcntl_old(fd, cmd, arg);
    } break;
    case F_GETFL:
    {
        va_end(va);
        int arg = fcntl_old(fd, cmd);

        hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(fd);
        // 避免对非套接字类型的文件描述符进行额外处理
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return arg;
        }
        // 根据用户的意图来保留或排除该标志位
        if (ctx->getUserNonblock()) {
            return arg | O_NONBLOCK;
        }
        else {
            return arg & ~O_NONBLOCK;
        }
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
#ifdef F_SETPIPE_SZ
    case F_SETPIPE_SZ:
#endif
    {
        int arg = va_arg(va, int);
        va_end(va);
        return fcntl_old(fd, cmd, arg);
    } break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
#ifdef F_GETPIPE_SZ
    case F_GETPIPE_SZ:
#endif
    {
        va_end(va);
        return fcntl_old(fd, cmd);
    } break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK:
    {
        struct flock* arg = va_arg(va, struct flock*);
        va_end(va);
        return fcntl_old(fd, cmd, arg);
    } break;
    case F_GETOWN_EX:
    case F_SETOWN_EX:
    {
        struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
        va_end(va);
        return fcntl_old(fd, cmd, arg);
    } break;
    default: va_end(va); return fcntl_old(fd, cmd);
    }
}

// 与设备进行交互和通信
int ioctl(int d, unsigned long int request, ...)
{
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    // 设置套接字为非阻塞模式
    if (FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;

        hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(d);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_old(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_old(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
{
    return getsockopt_old(sockfd, level, optname, optval, optlen);
}

// 设置套接字选项
int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
{
    if (!hiper::t_hook_enable) {
        return setsockopt_old(sockfd, level, optname, optval, optlen);
    }
    // 通用套接字
    if (level == SOL_SOCKET) {
        // 设置收发超时时间
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            hiper::FdCtx::ptr ctx = hiper::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_old(sockfd, level, optname, optval, optlen);
}
}
