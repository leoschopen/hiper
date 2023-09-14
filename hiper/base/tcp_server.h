/*
 * @Author: Leo
 * @Date: 2023-09-13 09:38:16
 * @LastEditTime: 2023-09-13 09:49:28
 * @Description: tcp 服务器
 */

#ifndef HIPER_TCP_SERVER_H
#define HIPER_TCP_SERVER_H

#include "address.h"
#include "config.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"

#include <functional>
#include <memory>

namespace hiper {

class TcpServer : public std::enable_shared_from_this<TcpServer>, public Noncopyable {
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(hiper::IOManager* worker        = hiper::IOManager::GetThis(),
              hiper::IOManager* accept_worker = hiper::IOManager::GetThis());

    virtual ~TcpServer();

    /**
     * @brief 绑定地址
     * @return 返回是否绑定成功
     */
    virtual bool bind(hiper::Address::ptr addr);

    /**
     * @brief 绑定地址数组
     * @param[in] addrs 需要绑定的地址数组
     * @param[out] fails 绑定失败的地址
     * @return 是否绑定成功
     */
    virtual bool bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails);

    /**
     * @brief 启动服务
     * @pre 需要bind成功后执行
     */
    virtual bool start();

    /**
     * @brief 停止服务
     */
    virtual void stop();

    /**
     * @brief 返回读取超时时间(毫秒)
     */
    uint64_t getRecvTimeout() const { return recv_timeout_; }

    /**
     * @brief 返回服务器名称
     */
    std::string getName() const { return server_name_; }

    /**
     * @brief 设置读取超时时间(毫秒)
     */
    void setRecvTimeout(uint64_t v) { recv_timeout_ = v; }

    /**
     * @brief 设置服务器名称
     */
    virtual void setName(const std::string& v) { server_name_ = v; }

    /**
     * @brief 是否停止
     */
    bool isStop() const { return is_stop_; }

    /**
     * @brief 以字符串形式dump server信息
     */
    virtual std::string toString(const std::string& prefix = "");

protected:
    /**
     * @brief 处理新连接的Socket类
     */
    virtual void handleClient(Socket::ptr client);

    /**
     * @brief 开始接受连接
     */
    virtual void startAccept(Socket::ptr sock);

protected:
    // 监听Socket数组
    std::vector<Socket::ptr> sockets_;
    // 新连接的Socket工作的调度器
    IOManager* io_worker_;
    // 服务器Socket接收连接的调度器
    IOManager* accept_worker_;
    // 接收超时时间(毫秒)
    uint64_t recv_timeout_;
    // 服务器名称
    std::string server_name_;
    // 服务器类型
    std::string type_;
    // 服务是否停止
    bool is_stop_;
};

}   // namespace hiper

#endif   // HIPER_TCP_SERVER_H
