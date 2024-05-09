#include "scheduler.h"

#include "log.h"
#include "macro.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    :m_name(name) {
    ORANGE_ASSERT(threads > 0);

    if(use_caller) {
        orange::Fiber::GetThis();
        --threads;

        ORANGE_ASSERT(t_scheduler == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        orange::Thread::SetName(m_name);

        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = orange::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    ORANGE_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    ORANGE_ASSERT(m_threads.empty());
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                            , m_name + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
}

void Scheduler::stop() {
    ORANGE_LOG_INFO(g_logger) << "Scheduler stop";
    m_autoStop = true;
    // no thread
    if(m_rootFiber
        && m_threadCount == 0
        && (m_rootFiber->getState() == Fiber::TERM
            || m_rootFiber->getState() == Fiber::INIT)) {
        ORANGE_LOG_INFO(g_logger) << this << " stopping";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    // bool exit_on_this_fiber = false;
    if(m_rootThread != -1) {
        ORANGE_ASSERT(GetThis() == this);
    } else {
        ORANGE_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }
    if(m_rootFiber) {
        if(!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto& i : thrs) {
        i->join();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    ORANGE_LOG_INFO(g_logger) << "run";
    setThis();
    if(orange::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = orange::Fiber::GetThis().get();
    }

    Fiber::ptr cb_fiber;
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

    FiberAndThread ft;
    while(true) {
        // Fiber::GetThis();
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;

        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                if(it->thread != -1 && it->thread != orange::GetThreadId()) {
                    tickle_me = true;
                    ++it;
                    continue;
                }

                ORANGE_ASSERT(it->fiber || it->cb);

                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= (it != m_fibers.end());
        }

        if(tickle_me) {
            tickle();
        }

        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                            && ft.fiber->getState() != Fiber::EXCPT)) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            }else if(ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCPT) {
                ft.fiber->setState(Fiber::HOLD);
            }
            ft.reset();
        } else if(ft.cb) {
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;

            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::TERM
                        || cb_fiber->getState() == Fiber::EXCPT) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }

            if(idle_fiber->getState() == Fiber::TERM) {
                ORANGE_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }
            // ORANGE_LOG_INFO(g_logger) << "idle fiber " << idle_fiber->getState();
            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;

            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCPT) {
                idle_fiber->setState(Fiber::HOLD);
            }
        }
    }
}

bool Scheduler::hasIdleThreads() {
    return m_idleThreadCount > 0;
}

void Scheduler::tickle() {
    // ORANGE_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    ORANGE_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        Fiber::YielToHold();
    }
}

}
