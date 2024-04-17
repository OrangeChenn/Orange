#include "src/orange.h"

#include <assert.h>

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

void test_assert() {
    ORANGE_LOG_INFO(g_logger) << orange::BacktraceToString(10, 2, "    ");
    ORANGE_ASSERT2(1 == 0, "chen orange");
}

int main(int argc, char** argv) {
    test_assert();
    return 0;
}
