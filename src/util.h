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

uint64_t GetCurrentMS();

uint64_t GetCurrentUS();

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");
}
