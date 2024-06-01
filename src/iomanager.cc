#include "iomanager.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"
#include "macro.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch (event) {
        case IOManager::Event::READ:
            return read;
        
        case IOManager::Event::WRITE:
            return write;
        
        default:
        ORANGE_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event) {
    ORANGE_ASSERT(events & event);

    events = (Event)(events & (~event));
    EventContext& event_ctx = getContext(event);
    if(event_ctx.cb) {
        event_ctx.scheduler->schedule(&event_ctx.cb);
    } else {
        event_ctx.scheduler->schedule(&event_ctx.fiber);
    }
    event_ctx.scheduler = nullptr;
}

// IOManager
IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller,  name) {
    m_epfd = epoll_create(5000);
    ORANGE_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFd);
    ORANGE_ASSERT(rt == 0);

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = m_tickleFd[0];
    event.events = EPOLLIN | EPOLLET;

    rt = fcntl(m_tickleFd[0], F_SETFL, fcntl(m_tickleFd[0], F_GETFL) | O_NONBLOCK);
    ORANGE_ASSERT(rt == 0);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFd[0], &event);
    ORANGE_ASSERT(rt == 0);

    contextResize(32);
    
    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFd[0]);
    close(m_tickleFd[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
            m_fdContexts[i] = nullptr;
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext();
            m_fdContexts[i]->fd = i;
        }
    }
}

// 0 success, , -1 error
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if(fd < (int)m_fdContexts.size()) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock3(fd_ctx->mutex);
    if(fd_ctx->events & event) {
        ORANGE_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " evnet=" << event
                    << " fd_ctx.event=" << fd_ctx->events;
        ORANGE_ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(0));
    // epevent.data.fd = fd;
    epevent.data.ptr = fd_ctx;
    epevent.events = EPOLLET | fd_ctx->events | event;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        ORANGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << epevent.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);

    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    ORANGE_ASSERT(!event_ctx.scheduler
                    && !event_ctx.fiber
                    && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        ORANGE_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    lock.unlock();
    FdContext* fd_ctx = m_fdContexts[fd];

    MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    // epevent.data.fd = fd;
    epevent.data.ptr = fd_ctx;
    epevent.events = EPOLLET | new_event;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        ORANGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << epevent.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    lock.unlock();
    FdContext* fd_ctx = m_fdContexts[fd];
    MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)) {
        return false;
    }

    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = EPOLLET | new_event;
    epevent.data.ptr = fd_ctx;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        ORANGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << epevent.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAllEvent(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    lock.unlock();
    FdContext* fd_ctx = m_fdContexts[fd];

    MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    memset(&epevent, 0, sizeof(epevent));
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        ORANGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " << epevent.events << "):"
                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    ORANGE_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if(!hasIdleThreads()) {
        return;
    }
    int rt = ::write(m_tickleFd[1], "T", 1);
    ORANGE_ASSERT(rt == 1);
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
            && m_pendingEventCount == 0
            && Scheduler::stopping();
}

void IOManager::idle() {
    ORANGE_LOG_INFO(g_logger) << "IOManager::idel()";
    static const int MAX_EVENTS = 64;
    epoll_event* events = new epoll_event[MAX_EVENTS]; // 栈内存较小
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(stopping(next_timeout)) {
            ORANGE_LOG_INFO(g_logger) << "name: " << getName()
                << ", idel stopping exit.";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT 
                    ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            if(rt < 0 && errno == EINTR) {
                continue;
            } else {
                // ORANGE_LOG_ERROR(g_logger) << "epoll_wait(" << m_epfd << ") (rt="
                //                         << rt << ") (errno=" << errno << ") (errstr:" << strerror(errno) << ")";
                break;
            }
        } while(true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);

        if(!cbs.empty()) {
            this->schedule(cbs.begin(), cbs.end());
            cbs.clear();
        } 

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFd[0]) {
                uint8_t dummy;
                while(::read(m_tickleFd[0], &dummy, 1) == 1); // ET仅通知一次,读干净
                continue;
            }
            FdContext* fd_ctx = (FdContext*)events[i].data.ptr;
            MutexType::Lock lock(fd_ctx->mutex);

            if(event.events & (EPOLLERR | EPOLLHUP)) { // why?
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }

            int real_event = NONE;
            if(event.events & EPOLLIN) {
                real_event |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_event |= WRITE;
            }

            if((event.events & real_event) == NONE) {
                continue;
            }

            int left_event = fd_ctx->events & ~real_event;
            int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_event;
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                ORANGE_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                            << op << ", " << event.events << "):"
                            << rt << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if(real_event & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_event & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        Fiber::ptr cur = Fiber::GetThis();
        Fiber* fiber = cur.get();
        cur.reset();
        fiber->swapOut();
    } // end while
}

void IOManager::onTimerInsertAtFront() {
    tickle();
}

}  // namespace orange
