/*
 * @Author: Leo
 * @Date: 2023-09-02 16:47:58
 * @LastEditTime: 2023-09-02 20:11:30
 * @Description:
 */

#ifndef HIPER_HOOK_H
#define HIPER_HOOK_H

#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace hiper {
// 细化到线程程度
bool is_hook_enabel();

void set_hook_enable(bool flag);
}   // namespace hiper



// 将指定的函数用C规则编译
extern "C" {

// 定义函数指针

// sleep
typedef unsigned int (*sleep_func)(unsigned int seconds);
extern sleep_func sleep_old; // 存储原库函数的地址

typedef int (*usleep_func)(useconds_t usec);
extern usleep_func usleep_old;

typedef int (*nanosleep_func)(const struct timespec* req, struct timespec* rem);
extern nanosleep_func nanosleep_old;

// socket
typedef int (*socket_func)(int domain, int type, int protocol);
extern socket_func socket_old;

typedef int (*connect_func)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
extern connect_func connect_old;

typedef int (*accept_func)(int s, struct sockaddr* addr, socklen_t* addrlen);
extern accept_func accept_old;

// read
typedef ssize_t (*read_func)(int fd, void* buf, size_t count);
extern read_func read_old;

typedef ssize_t (*readv_func)(int fd, const struct iovec* iov, int iovcnt);
extern readv_func readv_old;

typedef ssize_t (*recv_func)(int sockfd, void* buf, size_t len, int flags);
extern recv_func recv_old;

typedef ssize_t (*recvfrom_func)(int sockfd, void* buf, size_t len, int flags,
                                struct sockaddr* src_addr, socklen_t* addrlen);
extern recvfrom_func recvfrom_old;

typedef ssize_t (*recvmsg_func)(int sockfd, struct msghdr* msg, int flags);
extern recvmsg_func recvmsg_old;

// write
typedef ssize_t (*write_func)(int fd, const void* buf, size_t count);
extern write_func write_old;

typedef ssize_t (*writev_func)(int fd, const struct iovec* iov, int iovcnt);
extern writev_func writev_old;

typedef ssize_t (*send_func)(int s, const void* msg, size_t len, int flags);
extern send_func send_old;

typedef ssize_t (*sendto_func)(int s, const void* msg, size_t len, int flags,
                              const struct sockaddr* to, socklen_t tolen);
extern sendto_func sendto_old;

typedef ssize_t (*sendmsg_func)(int s, const struct msghdr* msg, int flags);
extern sendmsg_func sendmsg_old;

typedef int (*close_func)(int fd);
extern close_func close_old;

// 
typedef int (*fcntl_func)(int fd, int cmd, ... /* arg */);
extern fcntl_func fcntl_old;

typedef int (*ioctl_func)(int d, unsigned long int request, ...);
extern ioctl_func ioctl_old;

typedef int (*getsockopt_func)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
extern getsockopt_func getsockopt_old;

typedef int (*setsockopt_func)(int sockfd, int level, int optname, const void* optval,
                              socklen_t optlen);
extern setsockopt_func setsockopt_old;

extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen,
                                uint64_t timeout_ms);
}

#endif   // HIPER_HOOK_H
