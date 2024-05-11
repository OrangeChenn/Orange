#include "hook.h"

#include <dlfcn.h>

#include "iomanager.h"

namespace orange {

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_init = false;
    if(is_init) {
        return;
    }

/*
* 原始sleep
* sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
*/
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

struct _HookIniter {
    _HookIniter() {
        hook_init();
    }
};

static _HookIniter s_hook_initer; // 在程序执行之前构造，初始化hook_init


bool is_hook_enable() {
    return t_hook_enable;
}

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

} // namespace orange

extern "C" {

/*
* sleep_fun sleep_f = nullptr;
*/
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
    if(!orange::t_hook_enable) {
        return sleep_f(seconds);
    }
    orange::Fiber::ptr fiber = orange::Fiber::GetThis();
    orange::IOManager* iom = orange::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(orange::Scheduler::*)
            (orange::Fiber::ptr, int thread))&orange::IOManager::schedule,
            iom, fiber, -1));
    orange::Fiber::YielToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!orange::t_hook_enable) {
        return usleep_f(usec);
    }
    orange::Fiber::ptr fiber = orange::Fiber::GetThis();
    orange::IOManager* iom = orange::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(orange::Scheduler::*)
            (orange::Fiber::ptr, int thread))&orange::IOManager::schedule<orange::Fiber::ptr>,
            iom, fiber, -1));
    orange::Fiber::YielToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!orange::t_hook_enable) {
        return nanosleep_f(req, rem);
    }
    int time_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    orange::Fiber::ptr fiber = orange::Fiber::GetThis();
    orange::IOManager* iom = orange::IOManager::GetThis();
    iom->addTimer(time_ms, std::bind((void(orange::IOManager::*)
            (orange::Fiber::ptr, int thread))&orange::IOManager::schedule,
            iom, fiber, -1));
    orange::Fiber::YielToHold();
    return 0;
}

} // extern "C"
