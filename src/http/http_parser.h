#pragma once

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace orange {
namespace http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    
    HttpRequestParser();
    int isFinished();
    int hasError();
    size_t execute(char* data, size_t len);

    HttpRequest::ptr getData() const { return m_data; }
    void setError(int error) { m_error = error; }

    uint64_t getContentLength() const;
public:
    static uint64_t GetHttpRequestBufferSize();
    static uint64_t GetHttpRequestMaxBodySize();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    /// 错误码
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    bool m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;

    HttpResponseParser();
    int isFinished();
    int hasError();
    size_t execute(char* data, size_t len);

    HttpResponse::ptr getData() const { return m_data; }
    void setError(int error) { m_error = error; }

    uint64_t getContentLength() const;
public:
    static uint64_t GetResponseBufferSize();
    static uint64_t GetRespinseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    bool m_error;
};

} // namespace http
} // namespace orange
