#include "http_parser.h"

#include <string.h>

#include "src/config.h"
#include "src/log.h"

namespace orange {
namespace http {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

static orange::ConfigVar<uint64_t>::ptr g_http_request_buffer_size =
        orange::Config::Lookup<uint64_t>("http.requset.buffer_size"
                            ,4 * 1024ull, "http request buffer size");

static orange::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
        orange::Config::Lookup<uint64_t>("http.requset.max_body_size"
                            ,64 * 1024 * 1024ull, "http request max body size");

static orange::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
        orange::Config::Lookup<uint64_t>("http_response.buffer_size"
                            , 4 * 1024ull, "http response buffer size");

static orange::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
        orange::Config::Lookup<uint64_t>("http_response.max_body_size"
                            , 64 * 1024 * 1024ull, "http response max body size");

static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
static uint64_t s_http_response_buffer_size = 0;
static uint64_t s_http_response_max_body_size = 0;
// 避免getValue频繁调用影响性能
namespace {

struct _RequestSizeIniter {
    _RequestSizeIniter() {
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_request_max_body_size = g_http_response_max_body_size->getValue();

        g_http_request_buffer_size->addListener(
            [](const uint64_t& ov, const uint64_t& nv) {
                s_http_request_buffer_size = nv;
        });
        g_http_request_max_body_size->addListener(
            [](const uint64_t& ov, const uint64_t& nv) {
                s_http_request_max_body_size = nv;
        });
        g_http_response_buffer_size->addListener(
            [](const uint64_t& ov, const uint64_t& nv) {
                s_http_response_buffer_size = nv;
        });
        g_http_response_max_body_size->addListener(
            [](const uint64_t& ov, const uint64_t& nv) {
                s_http_response_max_body_size = nv;
        });
    }
};
static _RequestSizeIniter _init;

}

uint64_t HttpRequestParser::GetHttpRequestBufferSize() {
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize() {
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetResponseBufferSize() {
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetRespinseMaxBodySize() {
    return s_http_response_max_body_size;
}

void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod method = CharsToHttpMethod(at);
    if(method == HttpMethod::INVALID_METHOD) {
        ORANGE_LOG_WARN(g_logger) << "invalid http request method: "
            << std::string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(method);
}

void on_request_uri(void *data, const char *at, size_t length) {
    
}

void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if(strncmp("HTTP/1.1", at, length) == 0) {
        v = 0x11;
    } else if(strncmp("HTTP/1.0", at, length) == 0) {
        v = 0x10;
    } else {
        ORANGE_LOG_WARN(g_logger) << "invalide http request version: "
            << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_request_header_done(void *data, const char *at, size_t length) {
    // HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
}

void on_request_http_field(void *data, const char *field, size_t flen
                            ,const char *value, size_t vlen) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0) {
        ORANGE_LOG_WARN(g_logger) << "invalide http request field length = 0";
        // parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen)
                                ,std::string(value, vlen));
}

HttpRequestParser::HttpRequestParser()
    :m_error(0) {
    m_data = std::make_shared<HttpRequest>();
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;
    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
}

int HttpRequestParser::isFinished() {
    return http_parser_finish(&m_parser);
}

int HttpRequestParser::hasError() {
    return m_error | http_parser_has_error(&m_parser);
}

size_t HttpRequestParser::execute(char* data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset)); // 从 str2 复制 n 个字符到 str1
    return offset;
}

uint64_t HttpRequestParser::getContentLength() const {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at));
    parser->getData()->setStatus(status);
}

void on_response_chunk(void *data, const char *at, size_t length) {
    
}

void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strncmp("HTTP/1.1", at ,length) == 0) {
        v = 0x11;
    } else if(strncmp("HTTP/1.1", at ,length) == 0) {
        v = 0x10;
    } else {
        ORANGE_LOG_WARN(g_logger) << "invalide http response version: " 
                << std::string(at, length);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_response_header_done(void *data, const char *at, size_t length) {
    
}

void on_response_last_chunk(void *data, const char *at, size_t length) {
    
}

void on_response_http_field(void *data, const char *field, size_t flen
                            ,const char *value, size_t vlen) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0) {
        ORANGE_LOG_WARN(g_logger) << "invalide http response field length = 0";
        // parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(std::string(field, flen)
                                ,std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser()
    :m_error(0) {
    m_data = std::make_shared<HttpResponse>();
    httpclient_parser_init(&m_parser);
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
}

int HttpResponseParser::isFinished(){
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError() {
    return m_error | httpclient_parser_has_error(&m_parser);
}

size_t HttpResponseParser::execute(char* data, size_t len, bool chunk) {
    if(chunk) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

uint64_t HttpResponseParser::getContentLength() const {
    return m_data->getHeaderAs("Content-Length", 0);
}

} // namespace http
} // namespace orange