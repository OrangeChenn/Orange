#include "http_session.h"
#include "http_parser.h"

namespace orange {
namespace http {

HttpSession::HttpSession(orange::Socket::ptr socket, bool owner)
    :SocketStream(socket, owner){
}

HttpRequest::ptr HttpSession::recvResquest() {
    HttpRequestParser::ptr parser(new HttpRequestParser());
    uint64_t buffer_size = HttpRequestParser::GetHttpRequestBufferSize();
    // uint64_t buffer_size = 10;
    std::shared_ptr<char> buffer(new char[buffer_size], [](char* ptr) {
        delete[] ptr;
    });

    char* data = buffer.get();
    uint64_t offset = 0;
    do {
        int len = read(data + offset, buffer_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparser = parser->execute(data, len);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparser;
        if(parser->isFinished()) {
            break;
        }
        if(offset == buffer_size) {
            close();
            return nullptr;
        }
    } while(true);

    uint64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);
        int len = 0;
        if(length >= offset) { // 未读完
            memcpy(&body[0], data, offset);
            len = offset;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= offset;
        if(length > 0) {
            if(readFixSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    return parser->getData();
 }

int HttpSession::sendResponse(HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

} // namespace http
} // namespace orange
