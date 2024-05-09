#include "../src/orange.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();
int sock = 0;
void test_fiber() {
    ORANGE_LOG_INFO(g_logger) << "test_fiber() test";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "120.232.145.185", &addr.sin_addr.s_addr);
    ORANGE_LOG_INFO(g_logger) << "connect";

    // int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    // ORANGE_LOG_INFO(g_logger) << rt;

    // return;
    int rt = -10;
    if(!(rt = connect(sock, (const sockaddr*)&addr, sizeof(addr)))) {
        ORANGE_LOG_INFO(g_logger) << rt;
    } else if(errno == EINPROGRESS) {
        ORANGE_LOG_INFO(g_logger) << rt;
        ORANGE_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        orange::IOManager::GetThis()->addEvent(sock, orange::IOManager::READ, [](){
            ORANGE_LOG_INFO(g_logger) << "read callback";
        });
        orange::IOManager::GetThis()->addEvent(sock, orange::IOManager::WRITE, [](){
            ORANGE_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            orange::IOManager::GetThis()->cancelEvent(sock, orange::IOManager::READ);
            close(sock);
        });
    } else {
        ORANGE_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    } 
}

void test1() {
    orange::IOManager iom(2);
    iom.schedule(test_fiber); 
}

orange::Timer::ptr s_timer;
void test_timer() {
    orange::IOManager iom(2);
    s_timer = iom.addTimer(1000, []() {
        static int i = 0;
        ORANGE_LOG_INFO(g_logger) << "hello timer i =" << i;
        if(++i == 3) {
            s_timer->reset(2000, true);
        }
    }, true);
}

int main(int argc, char** argv) {
    // test1();
    test_timer();
    return 0;
}
