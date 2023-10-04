/*
 * @Author: Leo
 * @Date: 2023-10-03 15:17:49
 * @Description: 
 */

#include "socket_stream.h"

namespace hiper {

SocketStream::SocketStream(Socket::ptr sock, bool owner)
    : sock_(sock)
    , owner_(owner)
{}

SocketStream::~SocketStream()
{
    if (owner_ && sock_) {
        sock_->close();
    }
}

bool SocketStream::isConnected() const {
    return sock_ && sock_->isConnect();
}

int SocketStream::read(void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return sock_->recv(buffer, length);
}

int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    int rt = sock_->recv(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

int SocketStream::write(const void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return sock_->send(buffer, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getReadBuffers(iovs, length);
    int rt = sock_->send(&iovs[0], iovs.size());
    if(rt > 0) {
        ba->setPosition(ba->getPosition() + rt);
    }
    return rt;
}

void SocketStream::close() {
    if(sock_) {
        sock_->close();
    }
}

Address::ptr SocketStream::getRemoteAddress() {
    if(sock_) {
        return sock_->getRemoteAddress();
    }
    return nullptr;
}

Address::ptr SocketStream::getLocalAddress() {
    if(sock_) {
        return sock_->getLocalAddress();
    }
    return nullptr;
}

std::string SocketStream::getRemoteAddressString() {
    auto addr = getRemoteAddress();
    if(addr) {
        return addr->toString();
    }
    return "";
}

std::string SocketStream::getLocalAddressString() {
    auto addr = getLocalAddress();
    if(addr) {
        return addr->toString();
    }
    return "";
}

}