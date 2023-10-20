/*
 * @Author: Leo
 * @Date: 2023-10-04 11:32:21
 * @Description:
 */
#include "http_session.h"

#include "http_parser.h"


namespace hiper {
namespace http {

HttpSession::HttpSession(Socket::ptr sock, bool owner)
    : SocketStream(sock, owner)
{}

HttpRequest::ptr HttpSession::recvRequest()
{
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t               buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    std::shared_ptr<char>  buffer(new char[buff_size], [](char* ptr) { delete[] ptr; });
    char*                  data   = buffer.get();
    int                    offset = 0;
    do {
        // 从sock中循环接收请求数据并解析
        // len表示接收到的数据长度，offset表示已经解析的数据长度
        int len = read(data + offset, buff_size - offset);
        if (len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len);
        if (parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if (offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if (parser->isFinished()) {
            break;
        }
    } while (true);
    parser->getData()->init();
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}


}   // namespace http
}   // namespace hiper