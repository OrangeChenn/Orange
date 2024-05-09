#pragma once

#include <atomic>
#include <vector>

#include "scheduler.h"
#include "timer.h"

namespace orange {

class IOManager : public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE    = 0x0,
        READ    = 0x1,
        WRITE   = 0x4,
    };

private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr;
            Fiber::ptr fiber;
            std::function<void()> cb;
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;
        EventContext write;
        int fd = 0;
        Event events = NONE;
        MutexType mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller  = true, const std::string& name = "");
    ~IOManager();

    // 0 success, 1 retry, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);
    bool cancelAllEvent(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    bool stopping(uint64_t& timerout);
    void onTimerInsertAtFront() override;

private:
    void contextResize(size_t size);

private:
    int m_epfd = 0;
    int m_tickleFd[2]; // 0 read, 1 write

    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

} // namespace orange
