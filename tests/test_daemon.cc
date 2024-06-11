#include "src/daemon.h"
#include "src/iomanager.h"
#include "src/log.h"

static orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

orange::Timer::ptr timer;
int server_main(int argc, char** argv) {
    ORANGE_LOG_INFO(g_logger) << orange::ProcessInfoMrg::GetInstance()->toString();
    orange::IOManager iom(1);
    timer = iom.addTimer(1000, [](){
        ORANGE_LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if(++count == 10) {
            timer->cancel();
        }
    }, true);
    return 0;
}

int main(int argc, char** argv) {
    return orange::start_daemon(argc, argv, server_main, argc != 1);
}
