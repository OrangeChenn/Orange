#include "http_connection.h"
#include "http_parser.h"
#include "log.h"
#include "util.h"

namespace orange {
namespace http {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result=" << result
       << " error=" << error
       << " response=" << (response ? response->toString() : "")
       << "]";
    return ss.str();
}

HttpConnection::HttpConnection(orange::Socket::ptr socket, bool owner)
    :SocketStream(socket, owner){
    
}

HttpConnection::~HttpConnection() {
    ORANGE_LOG_DEBUG(g_logger) << "HttpConnectionPool::~HttpConnectionPool";
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser());
    uint64_t buffer_size = HttpResponseParser::GetResponseBufferSize();
    // uint64_t buffer_size = 10;
    std::shared_ptr<char> buffer(new char[buffer_size + 1], [](char* ptr) {
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
        data[len] = '\0';
        size_t nparser = parser->execute(data, len, false);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparser;
        if(offset == buffer_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);

    auto& client_parser = parser->getParser();
    if(client_parser.chunked) {
        std::string body;
        int len = offset;
        do {
            do {
                int rt = read(data + len, buffer_size - len);
                if(rt <= 0) {
                    close();
                    return nullptr;
                }
                len += rt;
                data[len] = '\0';
                size_t nparser = parser->execute(data, len, true);
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparser;
                if(len == (int)buffer_size) {
                    close();
                    return nullptr;
                }
            } while(!parser->isFinished());
            len -= 2; // "\r\n\r\n"
            if(client_parser.content_len <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len, len - client_parser.content_len);
                len -= client_parser.content_len;
            } else {
                body.append(data, len);
                int left = client_parser.content_len - len;
                while(left > 0) {
                    int rt = read(data, left > (int)buffer_size ? (int)buffer_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                len = 0;
            }
        } while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    } else {
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
    }
    return parser->getData();
 }

int HttpConnection::sendRequest(HttpRequest::ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

HttpResult::ptr HttpConnection::DoGet(const std::string& uristr
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    Uri::ptr uri = Uri::Create(uristr);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URI, nullptr
                , "invalid uri:" + uristr);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    return DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& uristr
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    Uri::ptr uri = Uri::Create(uristr);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URI, nullptr
                , "invalid uri:" + uristr);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    return DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                        , const std::string& uristr
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    Uri::ptr uri = Uri::Create(uristr);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URI, nullptr
                , "invalid uri:" + uristr);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                        , Uri::ptr uri
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                        , Uri::ptr uri
                        , int64_t timeout_ms) {
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr
                , "create address fail:invalid host" + uri->getHost());
    }
    Socket::ptr sock = Socket::CreateTCP(addr);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR, nullptr
                , "create socket fail:invalid host" + addr->toString());
    }
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL, nullptr
                , "connect socket fail:" + addr->toString());
    }
    sock->setRecvTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr
                , "send request closed by peer" + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr
                , "send request socket error, errno" + std::to_string(errno)
                + " strerror" + std::string(strerror(errno)));
    }
    HttpResponse::ptr rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr
                , "recv response timeout" + addr->toString() + " timeout=" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost
                , uint16_t port, uint32_t max_size
                , uint32_t max_alive_time, uint32_t max_request)
    :m_host(host)
    ,m_vhost(vhost)
    ,m_port(port)
    ,m_maxSize(max_size)
    ,m_maxAliveTime(max_alive_time)
    ,m_maxRequest(max_request) {
}

HttpConnection::ptr HttpConnectionPool::getConnection() {
    std::vector<HttpConnection*> invalid_conn;
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while(!m_conns.empty()) {
        HttpConnection* conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected()) {
            invalid_conn.push_back(conn);
            continue;
        }
        if(conn->m_createTime + m_maxAliveTime >= orange::GetCurrentMS()) {
            invalid_conn.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for(auto i : invalid_conn) {
        delete i;
    }
    m_total -= invalid_conn.size();
    if(!ptr) {
        orange::Address::ptr addr = orange::Address::LookupAnyIPAddress(m_host);
        if(!addr) {
            ORANGE_LOG_DEBUG(g_logger) << "create addr fail: " << m_host;
            return nullptr;
        }
        orange::Socket::ptr sock = orange::Socket::CreateTCP(addr);
        if(!sock) {
            ORANGE_LOG_DEBUG(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)) {
            ORANGE_LOG_DEBUG(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }
        ptr = new HttpConnection(sock);
        ++m_total;
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {
    ++ptr->m_request;
    if(!ptr->isConnected()
            || (ptr->m_createTime + pool->m_maxAliveTime >= orange::GetCurrentMS())
            || (ptr->m_request >= pool->m_maxRequest)) {
        delete ptr;
        --pool->m_total;
        return;
    }

    ++ptr->m_request;
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

HttpResult::ptr HttpConnectionPool::doGet(const std::string& uristr
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    return doRequest(HttpMethod::GET, uristr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (!uri->getQuery().empty() ? "?" : "")
       << uri->getQuery()
       << (!uri->getFragment().empty() ? "#" : "")
       << uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& uristr
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    return doRequest(HttpMethod::POST, uristr, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (!uri->getQuery().empty() ? "?" : "")
       << uri->getQuery()
       << (!uri->getFragment().empty() ? "#" : "")
       << uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                        , const std::string& uristr
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uristr);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        if(!m_vhost.empty()) {
            req->setHeader("Host", m_vhost);
        } else {
            req->setHeader("Host", m_host);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                        , Uri::ptr uri
                        , int64_t timeout_ms
                        , const std::map<std::string, std::string>& headers
                        , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (!uri->getQuery().empty() ? "?" : "")
       << uri->getQuery()
       << (!uri->getFragment().empty() ? "#" : "")
       << uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
                        , int64_t timeout_ms) {
    HttpConnection::ptr conn = getConnection();
    if(!conn) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr
                , "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    Socket::ptr sock = conn->getSocket();
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr
                , "pool host:" + m_host + " port:" + std::to_string(m_port));
    }
    sock->setRecvTimeout(timeout_ms);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr
                , "send request closed by peer: " + sock->getRemoteAddress()->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr
                , "send request socket error, errno" + std::to_string(errno)
                + " strerror" + std::string(strerror(errno)));
    }
    HttpResponse::ptr rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr
                , "recv response timeout" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

} // namespace http
} // namespace orange
