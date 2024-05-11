#include "hook.h"
#include "iomanager.h"
#include "log.h"

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test_sleep() {
    orange::IOManager iom(1);

    iom.schedule([]() {
        sleep(2);
        ORANGE_LOG_INFO(g_logger) << "sleep 2";
    });
    iom.schedule([]() {
        sleep(3);
        ORANGE_LOG_INFO(g_logger) << "sleep 3";
    });
    ORANGE_LOG_INFO(g_logger) << "test_sleep";
}

int main(int argc, char** argv) {
    test_sleep();
    return 0;
}
