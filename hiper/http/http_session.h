/*
 * @Author: Leo
 * @Date: 2023-10-04 11:02:39
 * @Description:基于HTTP协议的会话接口
 */

#ifndef HIPER_HTTP_SESSION_H
#define HIPER_HTTP_SESSION_H

#include "../streams/socket_stream.h"
#include "http.h"

namespace hiper {

namespace http {

class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;

    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接受HTTP请求并解析
     * 
     * @return HttpRequest::ptr 
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);
};

}   // namespace http

}   // namespace hiper

#endif