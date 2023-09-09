/*
 * @Author: Leo
 * @Date: 2023-09-08 09:10:24
 * @LastEditTime: 2023-09-09 11:50:04
 * @Description: 套接字类，表示一个套接字对象
 */

#ifndef HIPER_SOCKET_H
#define HIPER_SOCKET_H

#include "address.h"
#include "env.h"
#include "noncopyable.h"

#include <memory>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace hiper {

class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket>   weak_ptr;

    enum Type
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };

    enum Family
    {
        IPv4 = AF_INET,
        IPv6 = AF_INET6,
        UNIX = AF_UNIX
    };

    // int socket(int af, int type, int protocol);
    Socket(int family, int type, int protocol = 0);

    virtual ~Socket();

    // 将当前socket对象与地址addr进行绑定
    virtual bool bind(const Address::ptr addr);

    virtual bool listen(int backlog = 1024);

    virtual Socket::ptr accept();

    virtual bool close();

    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

    virtual bool reconnect(uint64_t timeout_ms = -1);

    virtual int send(const void* buffer, size_t length, int flags = 0);

    virtual int send(const iovec* buffers, size_t length, int flags = 0);

    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);

    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    virtual int recv(void* buffer, size_t length, int flags = 0);

    virtual int recv(iovec* buffers, size_t length, int flags = 0);

    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    const int getSocket() const { return sock_; };

    int getFamily() const { return family_; }

    int getType() const { return type_;}

    int getProtocol() const { return protocol_; }

    bool isConnect() const { return is_connect_; }

    const Address::ptr getLocalAddress();

    const Address::ptr getRemoteAddress();

    // socket是否有效
    bool isValid() const { return sock_ != -1; };

    int getError();

    bool getOption(int level, int option, void *result, socklen_t *len);

    template <class T>
    bool getOption(int level, int option, T &result) {
        socklen_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    bool setOption(int level, int optname, const void* optval, socklen_t optlen);

    template<typename T> bool setOption(int level, int optname, const T &optval)
    {
        return setOption(level, optname, &optval, sizeof(T));
    }

    virtual std::ostream &dump(std::ostream &os) const;

    virtual std::string toString() const;

    bool cancelRead();

    bool cancelWrite();

    bool cancelAccept();

    bool cancelAll();

    void setNonBlock();

    int64_t getSendTimeout();

    void setSendTimeout(int64_t v);

    int64_t getRecvTimeout();

    void setRecvTimeout(int64_t v);

public:
    // 创建socket，根据地址对象中的family，指定的协议，protocol默认值0创建
    // protocol 可以设置为0，表示根据family和type自动选择协议
    static Socket::ptr CreateTCP(hiper::Address::ptr address);

    static Socket::ptr CreateUDP(hiper::Address::ptr address);

    static Socket::ptr CreateTCPSocket();

    static Socket::ptr CreateUDPSocket();

    static Socket::ptr CreateTCPSocket6();

    static Socket::ptr CreateUDPSocket6();

    static Socket::ptr CreateUnixTCPSocket();

    static Socket::ptr CreateUnixUDPSocket();

protected:
    // 创建并初始化socket
    void newSocket();

    // 初始化socket，设置option
    void initSocket();

    /**
     * @brief 初始化socket，设置option以及本地和远程地址
     * 
     * @param sock 
     * @return true 
     * @return false 
     */
    virtual bool init(int sock);

private:
    int sock_     = -1;
    int family_   = -1;
    int type_     = -1;
    int protocol_ = 0;

    bool is_connect_ = false;

    Address::ptr local_addr_  = nullptr;
    Address::ptr remote_addr_ = nullptr;
};

std::ostream& operator<<(std::ostream& os, const Socket& sock);

}   // namespace hiper

#endif   // HIPER_SOCKET_H
