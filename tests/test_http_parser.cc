#include "src/http/http_parser.h"
#include "src/log.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

const char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: www.baidu.com\r\n"
                                "Content-length: 10\r\n\r\n"
                                "0123456789";

void test_request() {
    orange::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t size = parser.execute(&tmp[0], tmp.size());
    ORANGE_LOG_INFO(g_logger) << "execute rt = " << size
            << " has_error=" << parser.hasError()
            << " is_finished=" << parser.isFinished()
            << " total=" << tmp.size()
            << " content_length=" << parser.getContentLength()
            << " tmp[size]=" << tmp[size];

    tmp.resize(tmp.size() - size);
    ORANGE_LOG_INFO(g_logger) << "\r\n" << parser.getData()->toString();
    ORANGE_LOG_INFO(g_logger) << tmp;
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response() {
    orange::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t size = parser.execute(&tmp[0], tmp.size());
    ORANGE_LOG_INFO(g_logger) << "execute rt = " << size
            << " has_error=" << parser.hasError()
            << " is_finished=" << parser.isFinished()
            << " total=" << tmp.size()
            << " content_length=" << parser.getContentLength()
            << " tmp[size]=" << tmp[size];
    tmp.resize(tmp.size() - size);

    ORANGE_LOG_INFO(g_logger) << "\r\n" << parser.getData()->toString();
    ORANGE_LOG_INFO(g_logger) << tmp;
}

int main(int argc, char** argv) {
    test_request();
    ORANGE_LOG_INFO(g_logger) << "--------------------";
    test_response();
    return 0;
}
