#include "socket.h"

#include "hiper.h"
#include "log.h"
#include "macro.h"

#include <sys/socket.h>

namespace hiper {

static hiper::Logger::ptr g_logger = LOG_NAME("system");

Socket::Socket(int family, int type, int protocol)
    : sock_(-1)
    , family_(family)
    , type_(type)
    , protocol_(protocol)
    , is_connect_(false)
{}

Socket::~Socket()
{
    close();
}

void Socket::initSocket()
{
    int val = 1;

    // 允许地址复用
    setOption(SOL_SOCKET, SO_REUSEADDR, val);

    // // 禁用Nagle算法,减少小包的数量,让每条链接可同时有多个未确认的小包
    if (type_ == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSocket()
{
    sock_ = socket(family_, type_, protocol_);
    if (HIPER_LIKELY(sock_ != -1)) {
        initSocket();
    }
    else {
        LOG_ERROR(g_logger) << "socket(" << family_ << ", " << type_ << ", " << protocol_
                            << ") errno=" << errno << " errstr=" << strerror(errno);
    }
}

Socket::ptr Socket::CreateTCP(hiper::Address::ptr address)
{
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(hiper::Address::ptr address)
{
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSocket();
    sock->is_connect_ = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket()
{
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket()
{
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSocket();
    sock->is_connect_ = true;
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6()
{
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6()
{
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSocket();
    sock->is_connect_ = true;
    return sock;
}

Socket::ptr Socket::CreateUnixTCPSocket()
{
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUnixUDPSocket()
{
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}

int64_t Socket::getSendTimeout()
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock_);
    if (ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t v)
{
    struct timeval tv
    {
        v / 1000, (int)(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_SNDTIMEO, &tv);
}

int64_t Socket::getRecvTimeout()
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock_);
    if (ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t v)
{
    struct timeval tv
    {
        int(v / 1000), int(v % 1000 * 1000)
    };
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, socklen_t* len)
{
    int rt = getsockopt(sock_, level, option, result, (socklen_t*)len);
    if (rt) {
        LOG_DEBUG(g_logger) << "getOption sock=" << sock_ << " level=" << level
                            << " option=" << option << " errno=" << errno
                            << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}


bool Socket::setOption(int level, int option, const void* result, socklen_t len)
{
    int ret = setsockopt(sock_, level, option, result, len);
    if (HIPER_LIKELY(ret == 0)) {
        return true;
    }
    else {
        LOG_ERROR(g_logger) << "setsockopt(" << sock_ << ", " << level << ", " << option << ", "
                            << result << ", " << len << ") errno=" << errno
                            << " errstr=" << strerror(errno);
        return false;
    }
}

Socket::ptr Socket::accept()
{
    Socket::ptr sock(new Socket(family_, type_, protocol_));
    int         newsock = ::accept(sock_, nullptr, nullptr);
    if (newsock == -1) {
        LOG_ERROR(g_logger) << "accept(" << sock_ << ") errno=" << errno
                            << " errstr=" << strerror(errno);
        return nullptr;
    }
    if (HIPER_LIKELY(sock->init(newsock))) {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock)
{
    FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
    if (HIPER_UNLIKELY(!ctx->isSocket())) {
        errno = EBADF;
        return false;
    }
    if (HIPER_UNLIKELY(ctx->isClose())) {
        errno = EBADF;
        return false;
    }
    sock_ = sock;
    initSocket();
    getLocalAddress();
    getRemoteAddress();
    return true;
}

bool Socket::bind(const Address::ptr addr)
{
    local_addr_ = addr;
    if (!isValid()) {
        newSocket();
        if (HIPER_UNLIKELY(!isValid())) {
            return false;
        }
    }
    if (HIPER_UNLIKELY(addr->getFamily() != family_)) {
        LOG_ERROR(g_logger) << "bind sock.family(" << family_ << ") addr.family("
                            << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }

    // Unix socket地址直接bind可能会出现地址已被占用的情况,所以要进行特殊处理。
    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    // 创建一个临时的socket模拟客户端，如果连接成功，表示这个地址已经被使用
    if (uaddr) {
        Socket::ptr sock = Socket::CreateUnixTCPSocket();
        if (sock->connect(uaddr)) {
            return false;
        }
        else {
            hiper::FSUtil::Unlink(uaddr->getPath(), true);
        }
    }
    if (::bind(sock_, addr->getAddr(), addr->getAddrLen())) {
        LOG_ERROR(g_logger) << "bind error errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    getLocalAddress();
    return true;
}

bool Socket::reconnect(uint64_t timeout_ms)
{
    if (!remote_addr_) {
        LOG_ERROR(g_logger) << "reconnect remote_addr_ is null";
        return false;
    }

    local_addr_.reset();
    return connect(remote_addr_, timeout_ms);
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
    remote_addr_ = addr;
    if (!isValid()) {
        newSocket();
        if (HIPER_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if (HIPER_UNLIKELY(addr->getFamily() != family_)) {
        LOG_ERROR(g_logger) << "connect sock.family(" << family_ << ") addr.family("
                            << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }

    if (timeout_ms == (uint64_t)-1) {
        if (::connect(sock_, addr->getAddr(), addr->getAddrLen())) {
            LOG_ERROR(g_logger) << "sock_=" << sock_ << " connect(" << addr->toString()
                                << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    else {
        if (::connect_with_timeout(sock_, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            LOG_ERROR(g_logger) << "sock_=" << sock_ << " connect(" << addr->toString()
                                << ") timeout=" << timeout_ms << " error errno=" << errno
                                << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    is_connect_ = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}


bool Socket::listen(int backlog) {
    if (!isValid()) {
        LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    if (::listen(sock_, backlog)) {
        LOG_ERROR(g_logger) << "listen error errno=" << errno
                                  << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close() {
    if (!is_connect_ && sock_ == -1) {
        return true;
    }
    is_connect_ = false;
    if (sock_ != -1) {
        ::close(sock_);
        sock_ = -1;
    }
    return false;
}

int Socket::send(const void *buffer, size_t length, int flags) {
    if (isConnect()) {
        return ::send(sock_, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec *buffers, size_t length, int flags) {
    if (isConnect()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(sock_, &msg, flags);
    }
    return -1;
}

int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to, int flags) {
    if (isConnect()) {
        return ::sendto(sock_, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec *buffers, size_t length, const Address::ptr to, int flags) {
    if (isConnect()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov     = (iovec *)buffers;
        msg.msg_iovlen  = length;
        msg.msg_name    = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(sock_, &msg, flags);
    }
    return -1;
}

int Socket::recv(void *buffer, size_t length, int flags) {
    if (isConnect()) {
        return ::recv(sock_, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec *buffers, size_t length, int flags) {
    if (isConnect()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov    = (iovec *)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(sock_, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void *buffer, size_t length, Address::ptr from, int flags) {
    if (isConnect()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(sock_, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec *buffers, size_t length, Address::ptr from, int flags) {
    if (isConnect()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov     = (iovec *)buffers;
        msg.msg_iovlen  = length;
        msg.msg_name    = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(sock_, &msg, flags);
    }
    return -1;
}

const Address::ptr Socket::getRemoteAddress() {
    if (remote_addr_) {
        return remote_addr_;
    }

    Address::ptr result;
    switch (family_) {
    case AF_INET:
        result.reset(new IPv4Address());
        break;
    case AF_INET6:
        result.reset(new IPv6Address());
        break;
    case AF_UNIX:
        result.reset(new UnixAddress());
        break;
    default:
        result.reset(new UnknownAddress(family_));
        break;
    }
    socklen_t addrlen = result->getAddrLen();
    if (getpeername(sock_, result->getAddr(), &addrlen)) {
        LOG_ERROR(g_logger) << "getpeername error sock=" << sock_
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(family_));
    }
    if (family_ == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    remote_addr_ = result;
    return remote_addr_;
}

const Address::ptr Socket::getLocalAddress() {
    if (local_addr_) {
        return local_addr_;
    }

    Address::ptr result;
    switch (family_) {
    case AF_INET:
        result.reset(new IPv4Address());
        break;
    case AF_INET6:
        result.reset(new IPv6Address());
        break;
    case AF_UNIX:
        result.reset(new UnixAddress());
        break;
    default:
        result.reset(new UnknownAddress(family_));
        break;
    }
    socklen_t addrlen = result->getAddrLen();
    // 获取与给定套接字关联的本地地址信息（IP 地址和端口号）
    if (getsockname(sock_, result->getAddr(), &addrlen)) {
        LOG_ERROR(g_logger) << "getsockname error sock=" << sock_
                                  << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknownAddress(family_));
    }
    if (family_ == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    local_addr_ = result;
    return local_addr_;
}


int Socket::getError() {
    int error     = 0;
    socklen_t len = sizeof(error);
    if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
    os << "[Socket sock=" << sock_
       << " is_connected=" << is_connect_
       << " family=" << family_
       << " type=" << type_
       << " protocol=" << protocol_;
    if (local_addr_) {
        os << " local_address=" << local_addr_->toString();
    }
    if (remote_addr_) {
        os << " remote_address=" << remote_addr_->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() {
    return IOManager::GetThis()->cancelEvent(sock_, hiper::IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->cancelEvent(sock_, hiper::IOManager::WRITE);
}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->cancelEvent(sock_, hiper::IOManager::READ);
}

bool Socket::cancelAll() {
    return IOManager::GetThis()->cancelAll(sock_);
}

std::ostream &operator<<(std::ostream &os, const Socket &sock) {
    return sock.dump(os);
}

}   // namespace hiper