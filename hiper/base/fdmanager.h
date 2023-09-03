/*
 * @Author: Leo
 * @Date: 2023-09-02 16:28:02
 * @LastEditTime: 2023-09-02 16:34:11
 * @Description: 文件句柄管理器
 */

#ifndef HIPER_FDMANAGER_H
#define HIPER_FDMANAGER_H

#include "singleton.h"
#include "thread.h"

#include <memory>
#include <vector>

namespace hiper {

/**
 * @brief 文件句柄上下文类
 * @details 管理文件句柄类型(是否socket)
 *          是否阻塞,是否关闭,读/写超时时间
 */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    /**
     * @brief 通过文件句柄构造FdCtx
     */
    FdCtx(int fd);

    ~FdCtx() = default;


    bool isInit() const { return is_init_; }


    bool isSocket() const { return is_socket_; }


    bool isClose() const { return is_closed_; }

    /**
     * @brief 设置用户主动设置非阻塞
     * @param[in] v 是否阻塞
     */
    void setUserNonblock(bool v) { user_nonblock_ = v; }

    /**
     * @brief 获取是否用户主动设置的非阻塞
     */
    bool getUserNonblock() const { return user_nonblock_; }

    /**
     * @brief 设置系统非阻塞
     * @param[in] v 是否阻塞
     */
    void setSysNonblock(bool v) { sys_nonblock_ = v; }

    /**
     * @brief 获取系统非阻塞
     */
    bool getSysNonblock() const { return sys_nonblock_; }

    void setTimeout(int type, uint64_t v);

    /**
     * @brief 获取超时时间
     * @param[in] type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
     * @return 超时时间毫秒
     */
    uint64_t getTimeout(int type);

private:

    bool init();

private:
    // 是否初始化
    bool is_init_ : 1;
    // 是否socket
    bool is_socket_ : 1;
    // 是否hook非阻塞
    bool sys_nonblock_ : 1;
    // 是否用户主动设置非阻塞
    bool user_nonblock_ : 1;
    // 是否关闭
    bool is_closed_ : 1;
    // 文件句柄
    int fd_;
    // 读超时时间毫秒
    uint64_t recv_timeout_;
    // 写超时时间毫秒
    uint64_t send_timeout_;
};

/**
 * @brief 文件句柄管理类
 */
class FdManager {
public:
    typedef RWMutex RWMutexType;

    FdManager();

    /**
     * @brief 获取/创建文件句柄类FdCtx
     * @param[in] fd 文件句柄
     * @param[in] auto_create 是否自动创建
     * @return 返回对应文件句柄类FdCtx::ptr
     */
    FdCtx::ptr get(int fd, bool auto_create = false);

    void del(int fd);

private:
    // 读写锁
    RWMutexType mutex_;
    // 文件句柄集合
    std::vector<FdCtx::ptr> datas_;
};

/// 文件句柄单例
typedef Singleton<FdManager> FdMgr;

}   // namespace sylar

#endif   // HIPER_FDMANAGER_H
