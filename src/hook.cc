#include "hook.h"

#include <dlfcn.h>
#include <stdarg.h>

#include "config.h"
#include "fd_manager.h"
#include "log.h"
#include "iomanager.h"

orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

namespace orange {

static thread_local bool t_hook_enable = false;

static orange::ConfigVar<int>::ptr s_tcp_connect_timeout =
    orange::Config::Lookup("tcp.connect.timeouut", 5000, "tcp connect timeout");

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

static uint64_t s_connect_timeout = -1; 
struct _HookIniter {
    _HookIniter() {
        s_connect_timeout = (uint64_t)s_tcp_connect_timeout->getValue();
        s_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
            ORANGE_LOG_INFO(g_logger) << "tcp timeout changed from"
                                      << old_value << " to " << new_value;
        });
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


struct time_info {
    int cancelled = 0;
};

template<typename OringeFun, typename... Args>
static ssize_t do_io(int fd, OringeFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args... args) {
    if(!orange::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(fd);

    if(!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<time_info> tinfo(new time_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while(-1 == n && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if(-1 == n && errno == EAGAIN) {
        errno = 0;

        orange::IOManager* iom = orange::IOManager::GetThis();
        orange::Timer::ptr timer;

        /*
        * 可能io触发，提前执行当前协程，tinfo销毁
        */
        std::weak_ptr<time_info> winfo(tinfo);
        if((uint64_t)-1 != to) {
            timer = iom->addConditionTimer(to, [fd, winfo, iom, event]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }

                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (orange::IOManager::Event)event); //强制触发
            }, winfo);
        }

        // cb = nullptr，FdContext::EventContext.fiber=当前线程
        int rt = iom->addEvent(fd, (orange::IOManager::Event)(event));
        if(-1 == rt) {
            ORANGE_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                    << fd << ", " << event << ")";
            if(timer) {
                timer->cancel();
            }
            return -1;
        } else {
            orange::Fiber::YielToHold();

            // 1、addEvent数据回来触发
            // 2、超时时间触发

            if(timer) {
                timer->cancel();
            }

            if(tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }

            goto retry;
        }
    }

    return n;
}

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
            (orange::Fiber::ptr, int thread))&orange::IOManager::schedule,
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

int socket(int domain, int type, int protocol) {
    if(!orange::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(-1 == fd) {
        return fd;
    }
    orange::FdMrg::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr,
                        socklen_t addrlen, uint64_t timeout_ms) {
    if(!orange::t_hook_enable) {
        return connect_f(sockfd, addr, addrlen);
    }

    orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(sockfd);
    if(!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) {
        return connect_f(sockfd, addr, addrlen);
    }

    if(ctx->getUserNonblock()) {
        return connect_f(sockfd, addr, addrlen);
    }

    int n = connect_f(sockfd, addr, addrlen);
    if(0 == n) {
        return 0;
    } else if(-1 != n || errno != EINPROGRESS) {
        return n;
    }

    orange::IOManager* iom = orange::IOManager::GetThis();
    orange::Timer::ptr timer;
    std::shared_ptr<time_info> tinfo(new time_info);
    std::weak_ptr<time_info> winfo(tinfo);

    if((uint64_t)-1 != timeout_ms) {
        timer = iom->addConditionTimer(timeout_ms, [sockfd, iom, winfo]() {
            auto t = winfo.lock();
            if(!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(sockfd, orange::IOManager::WRITE);
        }, winfo);
    }

    int rt = iom->addEvent(sockfd, orange::IOManager::WRITE);
    
    if(-1 == rt) {
        ORANGE_LOG_ERROR(g_logger) << "addEvent(" << sockfd << ", "
                << orange::IOManager::WRITE << ")";
        if(timer) {
            timer->cancel();
        }
    } else {
        orange::Fiber::YielToHold();

        if(timer) {
            timer->cancel();
        }

        if(tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }

    int error = 0;
    socklen_t len = sizeof(int);

    if(-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }

    if(error == 0) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen,
            orange::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    if(!orange::t_hook_enable) {
        return accept_f(sockfd, addr, addrlen);
    }
    int fd = do_io(sockfd, accept_f, "accept", orange::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        orange::FdMrg::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", orange::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", orange::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", orange::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", orange::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", orange::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", orange::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", orange::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return do_io(sockfd, send_f, "send", orange::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return do_io(sockfd, sendto_f, "send", orange::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return do_io(sockfd, sendmsg_f, "sendmsg", orange::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!orange::t_hook_enable) {
        return close_f(fd);
    }
    orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(fd);
    if(ctx) {
        orange::IOManager* iom = orange::IOManager::GetThis();
        if(iom) {
            iom->cancelAllEvent(fd);
        }
        orange::FdMrg::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch (cmd)
    {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;

        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg |= O_NONBLOCK;
                } else {
                    return arg &= ~O_NONBLOCK;
                }
            }
            break;

        case F_SETFD:
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;

        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;

        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
            break;
    }
}

int ioctl(int fd, unsigned long request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg; // !!转为bool
        orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(fd);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!orange::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(SOL_SOCKET == level) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            orange::FdCtx::ptr ctx = orange::FdMrg::GetInstance()->get(sockfd);
            if(ctx) {
                const struct timeval* tv = (struct timeval*)optval;
                ctx->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

} // extern "C"
