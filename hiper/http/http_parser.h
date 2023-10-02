/*
 * @Author: Leo
 * @Date: 2023-09-22 17:49:05
 * @Description: http解析类
 */
#ifndef HIPER_HTTP_PARSER_H
#define HIPER_HTTP_PARSER_H

#include "http.h"

#include <memory>

namespace hiper {

namespace http {

/**
 * @brief HTTP请求解析类
 */
class HttpRequestParser {
public:
    /// HTTP解析类的智能指针
    typedef std::shared_ptr<HttpRequestParser> ptr;

    /**
     * @brief 构造函数
     */
    HttpRequestParser();

    /**
     * @brief 解析协议
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @return 返回实际解析的长度,并且将已解析的数据移除
     */
    size_t execute(char* data, size_t len);

    /**
     * @brief 是否解析完成
     * @return 是否解析完成
     */
    int isFinished() const { return finished_; }

    /**
     * @brief 设置是否解析完成
     */
    void setFinished(bool v) { finished_ = v; }

    /**
     * @brief 是否有错误
     * @return 是否有错误
     */
    int hasError() const { return !!error_; }

    /**
     * @brief 设置错误
     * @param[in] v 错误值
     */
    void setError(int v) { error_ = v; }

    /**
     * @brief 返回HttpRequest结构体
     */
    HttpRequest::ptr getData() const { return data_; }

    /**
     * @brief 获取http_parser结构体
     */
    const http_parser& getParser() const { return parser_; }

    /**
     * @brief 获取当前的HTTP头部field
     */
    const std::string& getField() const { return field_; }

    /**
     * @brief 设置当前的HTTP头部field
     */
    void setField(const std::string& v) { field_ = v; }

public:
    /**
     * @brief 返回HttpRequest协议解析的缓存大小
     */
    static uint64_t GetHttpRequestBufferSize();

    /**
     * @brief 返回HttpRequest协议的最大消息体大小
     */
    static uint64_t GetHttpRequestMaxBodySize();

private:
    /// http_parser
    http_parser parser_;
    /// HttpRequest
    HttpRequest::ptr data_;
    /// 错误码，参考http_errno
    int error_;
    /// 是否解析结束
    bool finished_;
    /// 当前的HTTP头部field，http-parser解析HTTP头部是field和value分两次返回
    std::string field_;
};

/**
 * @brief Http响应解析结构体
 */
class HttpResponseParser {
public:
    /// 智能指针类型
    typedef std::shared_ptr<HttpResponseParser> ptr;

    /**
     * @brief 构造函数
     */
    HttpResponseParser();

    /**
     * @brief 解析HTTP响应协议
     * @param[in, out] data 协议数据内存
     * @param[in] len 协议数据内存大小
     * @param[in] chunck 是否在解析chunck
     * @return 返回实际解析的长度,并且移除已解析的数据
     */
    size_t execute(char* data, size_t len);

    /**
     * @brief 是否解析完成
     */
    int isFinished() const { return finished_; }

    /**
     * @brief 设置是否解析完成
     */
    void setFinished(bool v) { finished_ = v; }

    /**
     * @brief 是否有错误
     */
    int hasError() const { return !!error_; }

    /**
     * @brief 设置错误码
     * @param[in] v 错误码
     */
    void setError(int v) { error_ = v; }

    /**
     * @brief 返回HttpResponse
     */
    HttpResponse::ptr getData() const { return data_; }

    /**
     * @brief 返回http_parser
     */
    const http_parser& getParser() const { return parser_; }

    /**
     * @brief 获取当前的HTTP头部field
     */
    const std::string& getField() const { return field_; }

    /**
     * @brief 设置当前的HTTP头部field
     */
    void setField(const std::string& v) { field_ = v; }

public:
    /**
     * @brief 返回HTTP响应解析缓存大小
     */
    static uint64_t GetHttpResponseBufferSize();

    /**
     * @brief 返回HTTP响应最大消息体大小
     */
    static uint64_t GetHttpResponseMaxBodySize();

private:
    /// HTTP响应解析器
    http_parser parser_;
    /// HTTP响应对象
    HttpResponse::ptr data_;
    /// 错误码
    int error_;
    /// 是否解析结束
    bool finished_;
    /// 当前的HTTP头部field
    std::string field_;
};

}   // namespace http

}   // namespace hiper

#endif   // HIPER_HTTP_PARSER_H