#pragma once

#include <assert.h>
#include <string.h>

#define ORANGE_ASSERT(x) \
    if(!(x)){ \
        ORANGE_LOG_ERROR(ORANGE_LOG_ROOT()) << "ASSERTION: " #x \
                << "\nbacktrace:\n" \
                << orange::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define ORANGE_ASSERT2(x, w) \
    if(!(x)){ \
        ORANGE_LOG_ERROR(ORANGE_LOG_ROOT()) << "ASSERTION: " #x \
                << "\n" << w \
                << "\nbacktrace:\n" \
                << orange::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
