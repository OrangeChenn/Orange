#include "tcp_server.h"
#include "bytearray.h"
#include "log.h"
#include "hook.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

class EchoServer : public orange::TcpServer {
public:
    typedef std::shared_ptr<EchoServer> ptr;
    EchoServer(int type)
        :m_type(type) {

    }

    void handleClient(orange::Socket::ptr sock) override {
        ORANGE_LOG_INFO(g_logger) << "handleClent...";
        orange::ByteArray::ptr ba(new orange::ByteArray());
        while(true) {
            std::vector<iovec> iovs;
            ba->getWriteBuffers(iovs, 1024);

            int rt = sock->recv(&iovs[0], iovs.size());
            // ORANGE_LOG_INFO(g_logger) << "rt = " << rt;
            if(rt == 0) {
                ORANGE_LOG_INFO(g_logger) << "sock close, sock=" << sock->getSocket();
                break;
            } else if(rt < 0) {
                ORANGE_LOG_ERROR(g_logger) << "sock recv error, errno=" << errno
                        << " strerror=" << strerror(errno);
                break;
            }
            ba->setPosition(ba->getPosition() + rt);
            ba->setPosition(0);
            if(m_type == 1) {
                std::cout << ba->toString() << std::endl;
            } else {
                std::cout << ba->toHexString() << std::endl;
            }
        }
    }
private:
    int m_type = 0;
};

int type = 0;

void run() {
    EchoServer::ptr echo_server(new EchoServer(type));
    orange::Address::ptr addr = orange::Address::LookupAny("0.0.0.0:8020");
    while(!echo_server->bind(addr)) {
        sleep(2);
    }
    echo_server->start();
}

int main(int argc, char** argv) {
    if(argc < 2) {
        ORANGE_LOG_ERROR(g_logger) << "use as[" << argv[0] << " -t or -b" << "]";
        return 0;
    }
    if(strcmp(argv[1], "-t") == 0) {
        type = 1;
    } else {
        type = 2;
    }

    orange::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
