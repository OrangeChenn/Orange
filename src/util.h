#pragma once

#include <pthread.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace orange {

pid_t GetThreadId();

int32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);

std::string BacktraceToString(int size, int skip = 2, const std::string prefix = "");

}
