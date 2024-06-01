#include "address.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <string.h>

#include <sstream>

#include "endian.hpp"
#include "log.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

template<class T>
static T CreateMask(uint32_t bits) {
    return ~((1 << (sizeof(T) * 8 - bits)) - 1);
}

template<class T>
static int32_t CountBytes(T value) {
    int32_t result = 0;
    for(; value; ++result) {
        value &= value - 1; // 11110000 & 11101111 = 11100000
    }
    return result;
}

// Address
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,
                int family, int type, int protocol) {
    addrinfo hints, *results, *next;
    hints.ai_addr = NULL;
    hints.ai_addrlen = 0;
    hints.ai_canonname = NULL;
    hints.ai_family = family;
    hints.ai_flags = 0;
    hints.ai_next = NULL;
    hints.ai_protocol = protocol;
    hints.ai_socktype = type;

    std::string node;
    const char* service = nullptr;

    if(!host.empty() && host[0] == '[') {
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(*(endipv6 + 1) == ']') {
            service = endipv6 + 2;
        }
        node = host.substr(1, endipv6 - host.c_str() - 1);
    }

    if(node.empty()) {
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
            }
            ++service;
        }
    }

    if(node.empty()) {
        node = host;
    }

    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        ORANGE_LOG_ERROR(g_logger) << "IPAddress::Lookup getaddrinfo(" << host << ", "
                                   << family << ", " << type  << ") errno = " << errno
                                   << ", gai_strerror = " << gai_strerror(error);
        return false;
    }
    next = results;
    while(next) {
        result.push_back(Create(next->ai_addr, next->ai_addrlen));
        next = next->ai_next;
    }
    freeaddrinfo(results);
    return true;
}

Address::ptr Address::LookupAny(const std::string& host,
                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        return result[0];
    }
    return nullptr;
}

IPAddress::ptr Address::LookupAnyIPAddress(const std::string& host,
                int family, int type, int protocol) {
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) {
        for(auto i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(i) {
                return v;
            }
        }
    }
    return nullptr;
}

bool Address::GetInterfaceAddress(std::multimap<std::string,
                std::pair<Address::ptr, uint32_t>>& result, int family) {
    struct ifaddrs *results, *next;

    int rt = getifaddrs(&results);
    if(rt != 0) {
        ORANGE_LOG_ERROR(g_logger) << "Address::GetInterfaceAddress family ="
                                   << family << ", errno=" << errno
                                   << ", strerrno" << strerror(errno);
        return false;
    }

    try {
        Address::ptr addr;
        uint32_t prefix_len = ~0u;

        for(next = results; next; next = next->ifa_next) {
            if(family != AF_UNSPEC && next->ifa_addr->sa_family) {
                continue;
            }
            switch (next->ifa_addr->sa_family)
            {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in));
                        uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBytes(netmask);
                    }
                    break;
                
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        // 掩码地址
                        struct in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        // 前缀长度，16字节挨个算
                        for (int i = 0; i < 16; ++i) {
                            prefix_len += CountBytes(netmask.__in6_u.__u6_addr8[i]);
                        }
                    }
                    break;

                default:
                    break;
            }

            if(addr) {
                result.insert(std::make_pair(next->ifa_name, std::make_pair(addr, prefix_len)));
            }
        }
    } catch(...) {
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return true;
}

bool Address::GetInterfaceAddress(std::vector<std::pair<Address::ptr,
                uint32_t>>& result, const std::string & iface, int family) {
    if(iface.empty() || iface == "*") {
        if(family == AF_INET || family == AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || AF_UNSPEC) {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string, std::pair<Address::ptr, uint32_t>> results;
    if(!GetInterfaceAddress(results, family)) {
        return false;
    }

    auto its = results.equal_range(iface);
    for(auto it = its.first; it != its.second; ++it) {
        result.push_back(it->second);
    }
    return true;
}

Address::ptr Address::Create(const sockaddr* address, socklen_t addr_len) {
    if(address == nullptr) {
        return nullptr;
    }
    Address::ptr result;
    switch (address->sa_family)
    {
        case AF_INET:
            result.reset(new IPv4Address(*(const struct sockaddr_in*)address));
            break;
        
        case AF_INET6:
            result.reset(new IPv6Address(*(const struct sockaddr_in6*)address));
            break;
        
        default:
            result.reset(new UnkownAddress(*address));
            break;
    }
    return result;
}

int Address::getFamily() const {
    return  getAddr()->sa_family;
}

std::string Address::toString() const {
    std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const {
    int minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        return true;
    } else if(result > 0) {
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const {
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const {
    return !(*this == rhs);
}

// IPAddress
IPAddress::ptr IPAddress::Create(const char* address, uint16_t port) {
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));
    
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, "http", &hints, &results);
    if(error) {
        ORANGE_LOG_ERROR(g_logger) << "IPAddress::Create(" << address << ", "
                                   << port  << ") errno = " << errno;
        return nullptr;
    }
    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
            Address::Create(results->ai_addr, results->ai_addrlen)
        );
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}

// IPv4Address
IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port) {
    IPv4Address::ptr rt(new IPv4Address());
    rt->m_addr.sin_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr.s_addr);
    if(result <= 0) {
        ORANGE_LOG_ERROR(g_logger) << "IPv4Address::Create(" << address << ", "
                                   << port << ") rt = " << result << ", errno="
                                   << errno << ", " << strerror(errno); 
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const struct sockaddr_in& addr) {
    m_addr = addr;
}

IPv4Address::IPv4Address(uint32_t address, uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address); // htonl(address)
    m_addr.sin_port = byteswapOnLittleEndian(port); // hotns(port);
}

const sockaddr* IPv4Address::getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const {
    return sizeof(m_addr);
}
std::ostream& IPv4Address::insert(std::ostream& os) const {
    // struct in_addr addr;
    // addr.s_addr = m_addr.sin_addr.s_addr;
    // inet_ntoa(addr);
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
    if(prefix_len > 32) {
        return nullptr;
    }
    sockaddr_in baddr(m_addr);
    // 主机号全为1
    baddr.sin_addr.s_addr |= ~byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in naddr(m_addr);
    naddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(naddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in subaddr;
    memset(&subaddr, 0, sizeof(subaddr));
    subaddr.sin_family = AF_INET;
    subaddr.sin_port = m_addr.sin_port;
    subaddr.sin_addr.s_addr = byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    ORANGE_LOG_DEBUG(g_logger) << subaddr.sin_addr.s_addr;
    return IPv4Address::ptr(new IPv4Address(subaddr));
}

uint16_t IPv4Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
}

void IPv4Address::setPort(uint16_t port) {
    m_addr.sin_port = byteswapOnLittleEndian(port);
}

// IPv6Address
IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port) {
    IPv6Address::ptr rt(new IPv6Address());
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr.__in6_u.__u6_addr8);
    if(result <= 0) {
        ORANGE_LOG_ERROR(g_logger) << "IPv6Address::Create(" << address << ", "
                                   << port << ") rt = " << result << ", errno="
                                   << errno << ", " << strerror(errno); 
        return nullptr;
    }
    return rt;
}

IPv6Address::IPv6Address() {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6& addr) {
    m_addr = addr;
}

IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin6_family = AF_INET6;
    m_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(m_addr.sin6_addr.__in6_u.__u6_addr8, address, 16);
}

const sockaddr* IPv6Address::getAddr() const {
    return (struct sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const {
    return sizeof(m_addr);
}
std::ostream& IPv6Address::insert(std::ostream& os) const {
    os << "[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.__in6_u.__u6_addr8;
    bool use_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !use_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !use_zeros) {
            os << ":";
            use_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }
    if(!use_zeros && addr[7] == 0) {
        os << "::";
    }
    os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.__in6_u.__u6_addr8[prefix_len / 8] |=
            ~CreateMask<uint32_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.__in6_u.__u6_addr8[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}

IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
    sockaddr_in6 naddr(m_addr);
    naddr.sin6_addr.__in6_u.__u6_addr8[prefix_len] &=
            CreateMask<uint32_t>(prefix_len % 8);
    for(int i = 0; i < 16; ++i) {
        naddr.sin6_addr.__in6_u.__u6_addr8[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(naddr));
}

IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_port = m_addr.sin6_port;
    subnet.sin6_addr.__in6_u.__u6_addr8[prefix_len / 8] =
            CreateMask<uint32_t>(prefix_len % 8);
    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.__in6_u.__u6_addr8[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}

uint16_t IPv6Address::getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}

void IPv6Address::setPort(uint16_t port) {
    m_addr.sin6_port = byteswapOnLittleEndian(port);
}

// UnixAddress
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

UnixAddress::UnixAddress(const std::string& address, uint32_t port) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_length = address.size() + 1;
    if(!address.empty() && address[0] == '\0') {
        --m_length;
    }
    if(m_length > MAX_PATH_LEN) {
        throw std::logic_error("path too long");
    }
    memcpy(&m_addr.sun_path, address.c_str(), m_length);
    m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const {
    return (struct sockaddr*)&m_addr;
}

void UnixAddress::setAddrLen(socklen_t v) {
    m_length = v;
}

socklen_t UnixAddress::getAddrLen() const {
    return sizeof(m_addr);
}
std::ostream& UnixAddress::insert(std::ostream& os) const {
    if(m_length > offsetof(sockaddr_un, sun_path) // 抽象 unix 域套接字地址
            && m_addr.sun_path[0] == '\0') {
        return os << "\0" << std::string(m_addr.sun_path + 1,
            m_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << m_addr.sun_path;
}

// UnkownAddress
UnkownAddress::UnkownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnkownAddress::UnkownAddress(const struct sockaddr& address) {
    m_addr = address;
}

const sockaddr* UnkownAddress::getAddr() const {
    return &m_addr;
}

socklen_t UnkownAddress::getAddrLen() const {
    return sizeof(m_addr);
}
std::ostream& UnkownAddress::insert(std::ostream& os) const {
    os << "[UnkownAddress family:" << m_addr.sa_family << "]";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    return addr.insert(os);
}

} // namespace orange
