#include "src/orange.h"

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test_scheduler() {
    static int s_count = 5;
    ORANGE_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if(s_count-- > 0) {
        orange::Scheduler::GetThis()->schedule(&test_scheduler, orange::GetThreadId());
    }
}

int main(int argc, char** argv) {
    orange::Thread::SetName("main");
    ORANGE_LOG_INFO(g_logger) << "main";
    orange::Scheduler sc(3, true, "test");
    sc.start();
    sleep(2);
    ORANGE_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_scheduler);
    ORANGE_LOG_INFO(g_logger) << "stop";
    sc.stop();
    ORANGE_LOG_INFO(g_logger) << "stop end";
    ORANGE_LOG_INFO(g_logger) << "over";
    return 0;
}
