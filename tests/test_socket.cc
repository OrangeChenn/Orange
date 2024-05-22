#include "socket.h"
#include "orange.h"
#include "iomanager.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test_socket() {
    orange::IPAddress::ptr addr = orange::Address::LookupAnyIPAddress("www.baidu.com");
    addr->setPort(80);
    if(addr) {
        ORANGE_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        ORANGE_LOG_ERROR(g_logger) << "get address fail";
    }

    orange::Socket::ptr sock = orange::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        ORANGE_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        ORANGE_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
        ORANGE_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }

    std::string buffer;
    buffer.resize(4096);
    rt = sock->recv(&buffer[0], buffer.size());
    if(rt <= 0) {
        ORANGE_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }
    buffer.resize(rt);
    ORANGE_LOG_INFO(g_logger) << buffer;
}

int main(int argc, char** argv) {
    orange::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}
