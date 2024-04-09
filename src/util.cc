#include "util.h"

#include <unistd.h>
#include <sys/syscall.h>

namespace orange {

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

int32_t GetFiberId() {
    return 0;
}

}
