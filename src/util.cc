#include "util.h"

#include <execinfo.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>

#include <iostream>

#include <filesystem>

#include "fiber.h"
#include "log.h"

namespace fs = std::filesystem;

namespace orange {

orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
}

int32_t GetFiberId() {
    return orange::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = ::backtrace_symbols(array, s);
    if(strings == NULL) {
        ORANGE_LOG_ERROR(g_logger) << "backtrace_symbols error";
        free(array);
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

uint64_t GetCurrentMS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

std::string Time2Str(time_t ts, const std::string& format) {
    struct tm tm;
    localtime_r(&ts, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), format.c_str(), &tm);
    return std::string(buf);
}

void FSUtil::ListAllFiles(std::vector<std::string>& files
            , const std::string& path, const std::string& filter) {
    if(!fs::exists(path) || !fs::is_directory(path)) {
        return;
    }
    for(const auto& entry : fs::recursive_directory_iterator(path)) {
        if(filter.empty() || 
                (entry.is_regular_file() && entry.path().extension() == filter)) {
            files.push_back(entry.path());
        }
    }
}

bool FSUtil::IsRunningPidfile(const std::string& pidfile) {
    if(!fs::exists(pidfile)) {
        return false;
    }
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) {
        return false;
    }
    if(line.empty()) {
        return false;
    }
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) {
        return false;
    }
    if(kill(pid, 0) != 0) {
        return false;
    }
    return true;
}

bool FSUtil::Mkdir(const std::string path) {
    if(fs::exists(path)) {
        return true;
    }
    try {
        fs::create_directories(path);
        return true;
    } catch(std::exception& e) {
        return false;   
    }
}

}
