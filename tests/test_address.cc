#include "address.h"
#include "log.h"
#include <map>

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test() {
    ORANGE_LOG_INFO(g_logger) << "test begin";
    std::vector<orange::Address::ptr> result;
    bool rt = orange::Address::Lookup(result, "www.baidu.com", AF_INET, SOCK_STREAM);
    if(!rt) {
        ORANGE_LOG_ERROR(g_logger) << "Lookup faile";
        return;
    }

    for(size_t i = 0; i < result.size(); ++i) {
        ORANGE_LOG_INFO(g_logger) << i << " " << result[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<orange::Address::ptr, uint32_t>> results;
    bool rt = orange::Address::GetInterfaceAddress(results);
    if(rt) {
        for(auto i : results) {
            ORANGE_LOG_INFO(g_logger) << i.first << "-" << i.second.first->toString() << "-" << i.second.second;
        }
    } else {
        ORANGE_LOG_ERROR(g_logger) << "GetInterfaceAddress fail";
    }
}

void test_ip() {
    orange::IPAddress::ptr addr = orange::IPAddress::Create("www.baidu.com", 80);
    if(addr) {
        ORANGE_LOG_INFO(g_logger) << addr->toString();
    }
}

void test_ipv4() {
    auto addr = orange::IPAddress::Create("10.1.20.3");
    auto saddr = addr->subnetMask(24);
    auto baddr = addr->broadcastAddress(24);
    auto naddr = addr->networdAddress(24);
    if(addr) {
        ORANGE_LOG_INFO(g_logger) << addr->toString();
    }
    if(saddr) {
        ORANGE_LOG_INFO(g_logger) << saddr->toString();
    }
    if(baddr) {
        ORANGE_LOG_INFO(g_logger) << baddr->toString();
    }
    if(naddr) {
        ORANGE_LOG_INFO(g_logger) << naddr->toString();
    }
}

void test_ipv6() {
    auto addr = orange::IPAddress::Create("fe80::215:5dff:fe20:e26a", 8020);
    if(addr) {
        ORANGE_LOG_INFO(g_logger) << addr->toString();
    }
    auto saddr = addr->subnetMask(64);
    auto baddr = addr->broadcastAddress(64);
    auto naddr = addr->networdAddress(64);
    if(saddr) {
        ORANGE_LOG_INFO(g_logger) << saddr->toString();
    }
    if(baddr) {
        ORANGE_LOG_INFO(g_logger) << baddr->toString();
    }
    if(naddr) {
        ORANGE_LOG_INFO(g_logger) << naddr->toString();
    }
}

int main(int argc, char** argv) {
    // test();
    // test_iface();
    // test_ip();
    // test_ipv4();
    test_ipv6();
    return 0;
}
