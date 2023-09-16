/*
 * @Author: Leo
 * @Date: 2023-09-15 09:09:02
 * @LastEditTime: 2023-09-15 11:14:13
 * @Description: HttpMethod HttpStatus HttpRequest HttpResponse
 */


#ifndef HIPER_HTTP_H
#define HIPER_HTTP_H

#include "http_parser.h"
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>
#include <boost/lexical_cast.hpp>
namespace hiper {
namespace http {


enum class HttpMethod
{
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

enum class HttpStatus
{
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};

/**
 * @brief 将字符串方法名转成HTTP方法枚举
 * @param[in] m HTTP方法
 * @return HTTP方法枚举
 */
const HttpMethod StringToHttpMethod(const std::string& methodStr);

/**
 * @brief 将字符串指针转换成HTTP方法枚举
 * @param[in] m 字符串方法枚举
 * @return HTTP方法枚举
 */
const HttpMethod CharsToHttpMethod(const char* m);

/**
 * @brief 将HTTP方法枚举转换成字符串
 * @param[in] m HTTP方法枚举
 * @return 字符串
 */
const std::string HttpMethodToString(HttpMethod method);

/**
 * @brief 将HTTP状态枚举转换成字符串
 * @param[in] m HTTP状态枚举
 * @return 字符串
 */
const std::string HttpStatusToString(const HttpStatus& s);

/**
 * @brief 忽略大小写比较仿函数
 */
struct CaseInsensitiveLess
{
    /**
     * @brief 忽略大小写比较字符串
     */
    bool operator()(const std::string& lhs, const std::string& rhs) const;
};

/**
 * @brief 获取Map中的key值,并转成对应类型,返回是否成功
 * @param[in] m Map数据结构
 * @param[in] key 关键字
 * @param[out] val 保存转换后的值
 * @param[in] def 默认值
 * @return
 *      @retval true 转换成功, val 为对应的值
 *      @retval false 不存在或者转换失败 val = def
 */
template<class MapType, class T>
bool checkGetAs(const MapType& m, const std::string& key, T& val, const T& def = T())
{
    auto it = m.find(key);
    if (it == m.end()) {
        val = def;
        return false;
    }
    try {
        val = boost::lexical_cast<T>(it->second);
        return true;
    }
    catch (...) {
        val = def;
    }
    return false;
}

/**
 * @brief 获取Map中的key值,并转成对应类型
 * @param[in] m Map数据结构
 * @param[in] key 关键字
 * @param[in] def 默认值
 * @return 如果存在且转换成功返回对应的值,否则返回默认值
 */
template<class MapType, class T>
T getAs(const MapType& m, const std::string& key, const T& def = T())
{
    auto it = m.find(key);
    if (it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<T>(it->second);
    }
    catch (...) {
    }
    return def;
}

class HttpResponse;
/**
 * @brief HTTP请求结构
 */
class HttpRequest {
public:
    /// HTTP请求的智能指针
    typedef std::shared_ptr<HttpRequest> ptr;

    /// MAP结构
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief 构造函数
     * @param[in] version 版本
     * @param[in] close 是否keepalive
     */
    HttpRequest(uint8_t version = 0x11, bool close = true);

    /**
     * @brief 从HTTP请求构造HTTP响应
     * @note 只需要保证请求与响应的版本号与keep-alive一致即可
     */
    std::shared_ptr<HttpResponse> createResponse();

    /**
     * @brief 返回HTTP方法
     */
    HttpMethod getMethod() const { return method_; }

    /**
     * @brief 返回HTTP版本
     */
    uint8_t getVersion() const { return version_; }

    /**
     * @brief 返回HTTP请求的路径
     */
    const std::string& getPath() const { return path_; }

    /**
     * @brief 返回HTTP请求的查询参数
     */
    const std::string& getQuery() const { return query_; }

    /**
     * @brief 返回HTTP请求的消息体
     */
    const std::string& getBody() const { return body_; }

    /**
     * @brief 返回HTTP请求的消息头MAP
     */
    const MapType& getHeaders() const { return headers_; }

    /**
     * @brief 返回HTTP请求的参数MAP
     */
    const MapType& getParams() const { return params_; }

    /**
     * @brief 返回HTTP请求的cookie MAP
     */
    const MapType& getCookies() const { return cookies_; }

    /**
     * @brief 设置HTTP请求的方法名
     * @param[in] v HTTP请求
     */
    void setMethod(HttpMethod v) { method_ = v; }

    /**
     * @brief 设置HTTP请求的协议版本
     * @param[in] v 协议版本0x11, 0x10
     */
    void setVersion(uint8_t v) { version_ = v; }

    /**
     * @brief 设置HTTP请求的路径
     * @param[in] v 请求路径
     */
    void setPath(const std::string& v) { path_ = v; }

    /**
     * @brief 设置HTTP请求的查询参数
     * @param[in] v 查询参数
     */
    void setQuery(const std::string& v) { query_ = v; }

    /**
     * @brief 设置HTTP请求的Fragment
     * @param[in] v fragment
     */
    void setFragment(const std::string& v) { fragment_ = v; }

    /**
     * @brief 设置HTTP请求的消息体
     * @param[in] v 消息体
     */
    void setBody(const std::string& v) { body_ = v; }

    /**
     * @brief 追加HTTP请求的消息体
     * @param[in] v 追加内容
     */
    void appendBody(const std::string& v) { body_.append(v); }

    /**
     * @brief 是否自动关闭
     */
    bool isClose() const { return close_; }

    /**
     * @brief 设置是否自动关闭
     */
    void setClose(bool v) { close_ = v; }

    /**
     * @brief 是否websocket
     */
    bool isWebsocket() const { return websocket_; }

    /**
     * @brief 设置是否websocket
     */
    void setWebsocket(bool v) { websocket_ = v; }

    /**
     * @brief 设置HTTP请求的头部MAP
     * @param[in] v map
     */
    void setHeaders(const MapType& v) { headers_ = v; }

    /**
     * @brief 设置HTTP请求的参数MAP
     * @param[in] v map
     */
    void setParams(const MapType& v) { params_ = v; }

    /**
     * @brief 设置HTTP请求的Cookie MAP
     * @param[in] v map
     */
    void setCookies(const MapType& v) { cookies_ = v; }

    /**
     * @brief 获取HTTP请求的头部参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string getHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 获取HTTP请求的请求参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string getParam(const std::string& key, const std::string& def = "");

    /**
     * @brief 获取HTTP请求的Cookie参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在则返回对应值,否则返回默认值
     */
    std::string getCookie(const std::string& key, const std::string& def = "");


    /**
     * @brief 设置HTTP请求的头部参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void setHeader(const std::string& key, const std::string& val);

    /**
     * @brief 设置HTTP请求的请求参数
     * @param[in] key 关键字
     * @param[in] val 值
     */

    void setParam(const std::string& key, const std::string& val);
    /**
     * @brief 设置HTTP请求的Cookie参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void setCookie(const std::string& key, const std::string& val);

    /**
     * @brief 删除HTTP请求的头部参数
     * @param[in] key 关键字
     */
    void delHeader(const std::string& key);

    /**
     * @brief 删除HTTP请求的请求参数
     * @param[in] key 关键字
     */
    void delParam(const std::string& key);

    /**
     * @brief 删除HTTP请求的Cookie参数
     * @param[in] key 关键字
     */
    void delCookie(const std::string& key);

    /**
     * @brief 判断HTTP请求的头部参数是否存在
     * @param[in] key 关键字
     * @param[out] val 如果存在,val非空则赋值
     * @return 是否存在
     */
    bool hasHeader(const std::string& key, std::string* val = nullptr);

    /**
     * @brief 判断HTTP请求的请求参数是否存在
     * @param[in] key 关键字
     * @param[out] val 如果存在,val非空则赋值
     * @return 是否存在
     */
    bool hasParam(const std::string& key, std::string* val = nullptr);

    /**
     * @brief 判断HTTP请求的Cookie参数是否存在
     * @param[in] key 关键字
     * @param[out] val 如果存在,val非空则赋值
     * @return 是否存在
     */
    bool hasCookie(const std::string& key, std::string* val = nullptr);

    /**
     * @brief 检查并获取HTTP请求的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[out] val 返回值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T> bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(headers_, key, val, def);
    }

    /**
     * @brief 获取HTTP请求的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T> T getHeaderAs(const std::string& key, const T& def = T())
    {
        return getAs(headers_, key, def);
    }

    /**
     * @brief 检查并获取HTTP请求的请求参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[out] val 返回值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T> bool checkGetParamAs(const std::string& key, T& val, const T& def = T())
    {
        initQueryParam();
        initBodyParam();
        return checkGetAs(params_, key, val, def);
    }

    /**
     * @brief 获取HTTP请求的请求参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T> T getParamAs(const std::string& key, const T& def = T())
    {
        initQueryParam();
        initBodyParam();
        return getAs(params_, key, def);
    }

    /**
     * @brief 检查并获取HTTP请求的Cookie参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[out] val 返回值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T> bool checkGetCookieAs(const std::string& key, T& val, const T& def = T())
    {
        initCookies();
        return checkGetAs(cookies_, key, val, def);
    }

    /**
     * @brief 获取HTTP请求的Cookie参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T> T getCookieAs(const std::string& key, const T& def = T())
    {
        initCookies();
        return getAs(cookies_, key, def);
    }

    /**
     * @brief 序列化输出到流中
     * @param[in, out] os 输出流
     * @return 输出流
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 转成字符串类型
     * @return 字符串
     */
    std::string toString() const;

    /**
     * @brief 提取url中的查询参数
     */
    void initQueryParam();

    /**
     * @brief 当content-type是application/x-www-form-urlencoded时，提取消息体中的表单参数
     */
    void initBodyParam();

    /**
     * @brief 提取请求中的cookies
     */
    void initCookies();

    /**
     * @brief 初始化，实际是判断connection是否为keep-alive，以设置是否自动关闭套接字
     */
    void init();

private:
    /// HTTP方法
    HttpMethod method_;
    /// HTTP版本
    uint8_t version_;
    /// 是否自动关闭
    bool close_;
    /// 是否为websocket
    bool websocket_;
    /// 参数解析标志位，0:未解析，1:已解析url参数, 2:已解析http消息体中的参数，4:已解析cookies
    uint8_t parserParamFlag_;
    /// 请求的完整url
    std::string url_;
    /// 请求路径
    std::string path_;
    /// 请求参数
    std::string query_;
    /// 请求fragment
    std::string fragment_;
    /// 请求消息体
    std::string body_;
    /// 请求头部MAP
    MapType headers_;
    /// 请求参数MAP
    MapType params_;
    /// 请求Cookie MAP
    MapType cookies_;
};

/**
 * @brief HTTP响应结构体
 */
class HttpResponse {
public:
    /// HTTP响应结构智能指针
    typedef std::shared_ptr<HttpResponse> ptr;

    /// MapType
    typedef std::map<std::string, std::string, CaseInsensitiveLess> MapType;

    /**
     * @brief 构造函数
     * @param[in] version 版本
     * @param[in] close 是否自动关闭
     */
    HttpResponse(uint8_t version = 0x11, bool close = true);

    /**
     * @brief 返回响应状态
     * @return 请求状态
     */
    HttpStatus getStatus() const { return status_; }

    /**
     * @brief 返回响应版本
     * @return 版本
     */
    uint8_t getVersion() const { return version_; }

    /**
     * @brief 返回响应消息体
     * @return 消息体
     */
    const std::string& getBody() const { return body_; }

    /**
     * @brief 返回响应原因
     */
    const std::string& getReason() const { return reason_; }

    /**
     * @brief 返回响应头部MAP
     * @return MAP
     */
    const MapType& getHeaders() const { return headers_; }

    /**
     * @brief 设置响应状态
     * @param[in] v 响应状态
     */
    void setStatus(HttpStatus v) { status_ = v; }

    /**
     * @brief 设置响应版本
     * @param[in] v 版本
     */
    void setVersion(uint8_t v) { version_ = v; }

    /**
     * @brief 设置响应消息体
     * @param[in] v 消息体
     */
    void setBody(const std::string& v) { body_ = v; }

    /**
     * @brief 追加HTTP请求的消息体
     * @param[in] v 追加内容
     */
    void appendBody(const std::string& v) { body_.append(v); }

    /**
     * @brief 设置响应原因
     * @param[in] v 原因
     */
    void setReason(const std::string& v) { reason_ = v; }

    /**
     * @brief 设置响应头部MAP
     * @param[in] v MAP
     */
    void setHeaders(const MapType& v) { headers_ = v; }

    /**
     * @brief 是否自动关闭
     */
    bool isClose() const { return close_; }

    /**
     * @brief 设置是否自动关闭
     */
    void setClose(bool v) { close_ = v; }

    /**
     * @brief 是否websocket
     */
    bool isWebsocket() const { return websocket_; }

    /**
     * @brief 设置是否websocket
     */
    void setWebsocket(bool v) { websocket_ = v; }

    /**
     * @brief 获取响应头部参数
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在返回对应值,否则返回def
     */
    std::string getHeader(const std::string& key, const std::string& def = "") const;

    /**
     * @brief 设置响应头部参数
     * @param[in] key 关键字
     * @param[in] val 值
     */
    void setHeader(const std::string& key, const std::string& val);

    /**
     * @brief 删除响应头部参数
     * @param[in] key 关键字
     */
    void delHeader(const std::string& key);

    /**
     * @brief 检查并获取响应头部参数
     * @tparam T 值类型
     * @param[in] key 关键字
     * @param[out] val 值
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回true,否则失败val=def
     */
    template<class T> bool checkGetHeaderAs(const std::string& key, T& val, const T& def = T())
    {
        return checkGetAs(headers_, key, val, def);
    }

    /**
     * @brief 获取响应的头部参数
     * @tparam T 转换类型
     * @param[in] key 关键字
     * @param[in] def 默认值
     * @return 如果存在且转换成功返回对应的值,否则返回def
     */
    template<class T> T getHeaderAs(const std::string& key, const T& def = T())
    {
        return getAs(headers_, key, def);
    }

    /**
     * @brief 序列化输出到流
     * @param[in, out] os 输出流
     * @return 输出流
     */
    std::ostream& dump(std::ostream& os) const;

    /**
     * @brief 转成字符串
     */
    std::string toString() const;

    /**
     * @brief 设置重定向，在头部添加Location字段，值为uri
     * @param[] uri 目标uri
     */
    void setRedirect(const std::string& uri);

    /**
     * @brief 为响应添加cookie
     * @param[] key cookie的key值
     * @param[] val cookie的value
     * @param[] expired 过期时间
     * @param[] path cookie的影响路径
     * @param[] domain cookie作用的域
     * @param[] secure 安全标志
     */
    void setCookie(const std::string& key, const std::string& val, time_t expired = 0,
                   const std::string& path = "", const std::string& domain = "",
                   bool secure = false);

private:
    /// 响应状态
    HttpStatus status_;
    /// 版本
    uint8_t version_;
    /// 是否自动关闭
    bool close_;
    /// 是否为websocket
    bool websocket_;
    /// 响应消息体
    std::string body_;
    /// 响应原因
    std::string reason_;
    /// 响应头部MAP
    MapType headers_;
    /// cookies
    std::vector<std::string> cookies_;
};

/**
 * @brief 流式输出HttpRequest
 * @param[in, out] os 输出流
 * @param[in] req HTTP请求
 * @return 输出流
 */
std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

/**
 * @brief 流式输出HttpResponse
 * @param[in, out] os 输出流
 * @param[in] rsp HTTP响应
 * @return 输出流
 */
std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);

}   // namespace http
}   // namespace hiper

#endif   // HIPER_HTTP_H