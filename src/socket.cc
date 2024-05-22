#include "socket.h"

#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "fd_manager.h"
#include "hook.h"
#include "iomanager.h"
#include "log.h"
#include "macro.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(Address::ptr addr) {
    Socket::ptr sock(new Socket(addr->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(Address::ptr addr) {
    Socket::ptr sock(new Socket(addr->getFamily(), UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPUnixSocket() {
    Socket::ptr sock(new Socket(Unix, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPUnixSocket() {
    Socket::ptr sock(new Socket(Unix, UDP, 0));
    return sock;
}

Socket::Socket(int family, int type, int protocol) 
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isConnected(false) {
}

Socket::~Socket() {
    close();
}

int64_t Socket::getSendTimeout() const {
    orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}

void Socket::setSendTimeout(int64_t timeout_ms) {
    struct timeval tv{(int)(timeout_ms / 1000), (int)(timeout_ms % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, &tv);
}

int64_t Socket::getRecvTimeout() const {
    orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeout(int64_t timeout_ms) {
    struct timeval tv{(int)(timeout_ms / 1000), (int)(timeout_ms % 1000 / 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, &tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        ORANGE_LOG_DEBUG(g_logger) << "Socket::getOption m_sock=" << m_sock
            << ", level=" << level << ", option=" << option
            << ", errno=" << errno << ", strerrno=" << strerror(errno);
        return false;
    }
    return true;
}

void Socket::setOption(int level, int option, const void* result, size_t len) {
    int rt = setsockopt(m_sock, level, option, result, (socklen_t)len);
    if(rt) {
        ORANGE_LOG_DEBUG(g_logger) << "Socket::setOption m_sock=" << m_sock
            << ", level=" << level << ", option=" << option
            << ", errno=" << errno << ", strerrno=" << strerror(errno);
    }
}

Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        ORANGE_LOG_DEBUG(g_logger) << "Socket::accept(" << m_sock << ") errno="
                << errno << " strerrno=" << strerror(errno);
        return nullptr;
    }
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock) {
    FdCtx::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(sock);
    if(!ctx && ctx->isSocket() && !ctx->isClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

bool Socket::bind(const Address::ptr addr) {
    if(ORANGE_UNLIKELY(!isValid())) {
        newSocket();
        if(ORANGE_UNLIKELY(!isValid())) {
            return false;
        }
    }

    if(ORANGE_UNLIKELY(addr->getFamily() != m_family)) {
        ORANGE_LOG_DEBUG(g_logger) << "bind addr.family(" << addr->getFamily()
                << ") sock.family(" << m_family << ") not equal, addr="
                << addr->toString();
        return false;
    }
    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        ORANGE_LOG_ERROR(g_logger) << "bind error=" << errno
                << " strerror=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    if(ORANGE_UNLIKELY(!isValid())) {
        newSocket();
        if(ORANGE_UNLIKELY(!isValid())) {
            ORANGE_LOG_ERROR(g_logger) << "connect addr.family(" << addr->getFamily()
                << ") sock.family(" << m_family << ") not equal, addr="
                << addr->toString();
            return false;
        }
    }

    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            ORANGE_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") timeout=" << timeout_ms << " errno=" << errno
                    << " strerror=" << strerror(errno);
            close();
            return false;
        }
    } else {
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            ORANGE_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                    << ") timeout=" << timeout_ms << " errno=" << errno
                    << " strerror=" << strerror(errno);
            close();
            return false;
        }
    }
    m_isConnected = true;
    getLocalAddress();
    getRemoteAddress();
    return true;
}

bool Socket::listen(int backlog) {
    if(ORANGE_UNLIKELY(!isValid())) {
        ORANGE_LOG_ERROR(g_logger) << "listen error sock = -1";
        return false;
    }
    if(::listen(m_sock, backlog)) {
        ORANGE_LOG_ERROR(g_logger) << "listen error errno=" << errno
                << " strerror=" << strerror(errno);
        return false;
    }
    return true;
}

bool Socket::close() {
    if(!m_isConnected && -1 == m_sock) {
        return true;
    }
    m_isConnected = false;
    if(-1 != m_sock) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec* buffer, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        msg.msg_name = const_cast<sockaddr*>(to->getAddr());
        msg.msg_namelen= to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec* buffer, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, const_cast<sockaddr*>(from->getAddr()), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        msg.msg_name = const_cast<sockaddr*>(from->getAddr());
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }
    Address::ptr result;
    switch (m_family)
    {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;   
        default:
            result.reset(new UnkownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, const_cast<sockaddr*>(result->getAddr()), &addrlen)) {
        ORANGE_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
            << " errno=" << errno << " strerror=" << strerror(errno);
        return Address::ptr(new UnkownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }
    Address::ptr result;
    switch (m_family)
    {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;   
        default:
            result.reset(new UnkownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock, const_cast<sockaddr*>(result->getAddr()), &addrlen)) {
        ORANGE_LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
            << " errno=" << errno << " strerror=" << strerror(errno);
        return Address::ptr(new UnkownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}

bool Socket::isValid() const {
    return m_sock != -1;
}

int Socket::getError() {
    int error = 0;
    size_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol
       << " isConnected=" << m_isConnected;
    
    if(m_localAddress) {
        os << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

bool Socket::cancelRead() {
    return orange::IOManager::GetThis()->cancelEvent(m_sock, orange::IOManager::Event::READ);
}

bool Socket::cancelWrite() {
    return orange::IOManager::GetThis()->cancelEvent(m_sock, orange::IOManager::Event::WRITE);
}

bool Socket::cancelAccept() {
    return orange::IOManager::GetThis()->cancelEvent(m_sock, orange::IOManager::Event::READ);
}

bool Socket::cancelAll() {
    return orange::IOManager::GetThis()->cancelAllEvent(m_sock);
}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}

void Socket::newSocket() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(ORANGE_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        ORANGE_LOG_DEBUG(g_logger) << "Socket(" << m_family << ", "
                << m_type << ", " << m_protocol << ") errno=" << errno
                << ", strerror=" << strerror(errno);
    }
}

} // namespace orange
