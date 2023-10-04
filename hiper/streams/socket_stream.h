/*
 * @Author: Leo
 * @Date: 2023-10-03 15:14:02
 * @Description: Socket流式接口
 */

#ifndef HIPER_SOCKET_STREAM_H
#define HIPER_SOCKET_STREAM_H

#include "../base/socket.h"
#include "../base/stream.h"

#include <string>

namespace hiper {

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;

    SocketStream(Socket::ptr sock, bool owner = true);

    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;

    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;

    virtual void close() override;

    Socket::ptr getSocket() const { return sock_; }

    bool isConnected() const;

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();
    std::string  getRemoteAddressString();
    std::string  getLocalAddressString();

protected:
    Socket::ptr sock_;
    bool        owner_;  // 是否需要释放socket
};

}   // namespace hiper

#endif
