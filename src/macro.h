#pragma once

#include <assert.h>
#include <string.h>

#if defined __GUNS__ || defined __llvm__
#   define ORANGE_LIKELY(x)         __builtin_expect(!!(x), 1)
#   define ORANGE_UNLIKELY(x)       __builtin_expect(!!(x), 0)
#else
#   define ORANGE_LIKELY(X)         (X)
#   define ORANGE_UNLIKELY(x)       (x)
#endif

#define ORANGE_ASSERT(x) \
    if(ORANGE_UNLIKELY(!(x))){ \
        ORANGE_LOG_ERROR(ORANGE_LOG_ROOT()) << "ASSERTION: " #x \
                << "\nbacktrace:\n" \
                << orange::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define ORANGE_ASSERT2(x, w) \
    if(ORANGE_UNLIKELY(!(x))){ \
        ORANGE_LOG_ERROR(ORANGE_LOG_ROOT()) << "ASSERTION: " #x \
                << "\n" << w \
                << "\nbacktrace:\n" \
                << orange::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
