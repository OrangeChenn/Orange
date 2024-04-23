#pragma once

#include <ucontext.h>

#include <memory>
#include <functional>
#include "mutex.h"

namespace orange {

class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {
friend Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCPT
    };

private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stackSize = 0, bool use_call = false);
    ~Fiber();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }
    void setState(Fiber::State state) { m_state = state; }
    void reset(std::function<void()> cb);

    void call();
    void back();
    void swapIn();
    void swapOut();

public:
    static uint64_t GetFiberId();
    static void SetThis(Fiber* fp);
    static Fiber::ptr GetThis();
    static void YielToReady();
    static void YielToHold();

    static uint64_t TotalFibers();
    static void MainFunc();
    static void CallerMainFunc();

private:
    uint64_t m_id = 0;
    uint32_t m_stackSize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
}; 

}
