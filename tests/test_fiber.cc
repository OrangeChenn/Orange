#include "src/fiber.h"
#include "src/log.h"

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void run_in_fiber() {
    ORANGE_LOG_INFO(g_logger) << "run_in_fiber begin";
    orange::Fiber::YielToHold();
    ORANGE_LOG_INFO(g_logger) << "run_in_fiber end";
    orange::Fiber::YielToHold();
}

void test_fiber() {
    {
        ORANGE_LOG_INFO(g_logger) << "main begin";
        orange::Fiber::GetThis();
        orange::Fiber::ptr fiber(new orange::Fiber(run_in_fiber));
        fiber->swapIn();
        ORANGE_LOG_INFO(g_logger) << "main fiber swapIn";
        fiber->swapIn();
        ORANGE_LOG_INFO(g_logger) << "main fiber swapIn2";
        fiber->swapIn();
    }

    ORANGE_LOG_INFO(g_logger) << "main end";
}

int main(int argc, char** argv) {
    orange::Thread::SetName("main");
    std::vector<orange::Thread::ptr> thrs;
    for(int i = 0; i < 3; i++) {
        thrs.push_back(orange::Thread::ptr(
                new orange::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    for(auto i : thrs) {
        i->join();
    }

    return 0;
}
