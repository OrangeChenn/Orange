#include "fiber.h"

#include <atomic>
#include <exception>

#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

namespace orange {

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local std::shared_ptr<Fiber> t_fiberThread = nullptr;

static orange::ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    orange::Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);
    
    if(getcontext(&m_ctx)) {
        ORANGE_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
    // ORANGE_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
 }

Fiber::Fiber(std::function<void()> cb, size_t stackSize /* = 0 */, bool use_call /* = false */) 
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    m_state = INIT;
    m_stackSize = stackSize != 0 ? stackSize : g_fiber_stack_size->getValue();

    if(getcontext(&m_ctx)) {
        ORANGE_ASSERT2(false, "getcontext");
    }
    m_stack = StackAllocator::Alloc(m_stackSize);
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;

    if(use_call) {
        makecontext(&m_ctx, Fiber::CallerMainFunc, 0);
    } else {
        makecontext(&m_ctx, Fiber::MainFunc, 0);
    }
    // ORANGE_LOG_DEBUG(g_logger) << "Fiber::Fiber(x, x) id = " << m_id;
}

Fiber::~Fiber() {
    // ORANGE_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
    --s_fiber_count;
    if(m_stack) {
        ORANGE_ASSERT(m_state == INIT
                || m_state == EXCPT
                || m_state == TERM);
        StackAllocator::Dealloc(m_stack, m_stackSize);
    } else {
        ORANGE_ASSERT(!m_cb);
        ORANGE_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
}

void Fiber::reset(std::function<void()> cb) {
    ORANGE_ASSERT(m_stack);
    ORANGE_ASSERT(m_state == INIT
            || m_state == EXCPT
            || m_state == TERM);
    
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        ORANGE_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stackSize;
    makecontext(&m_ctx, Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call() {
    SetThis(this);
    ORANGE_ASSERT(m_state != EXEC);
    m_state = EXEC;
    
    if(swapcontext(&(t_fiberThread->m_ctx), &m_ctx)) {
        ORANGE_ASSERT2(false, "call swapcontext");
    }
}

void Fiber::back() {
    SetThis(t_fiberThread.get());
    if(swapcontext(&m_ctx, &(t_fiberThread->m_ctx))) {
        ORANGE_ASSERT2(false, "back swapcontext");
    }
}

void Fiber::swapIn() {
    SetThis(this);
    ORANGE_ASSERT(m_state != EXEC);
    m_state = EXEC;

    if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
        ORANGE_ASSERT2(false, "swapin swapcontext");
    }
}

void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
        ORANGE_ASSERT2(false, "swapout swapcontext");
    }
}

uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

void Fiber::SetThis(Fiber* fp) {
    t_fiber = fp;
}

Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    ORANGE_ASSERT(t_fiber == main_fiber.get());
    t_fiberThread = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YielToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YielToHold() {
    Fiber::ptr cur = GetThis();
    ORANGE_ASSERT(cur->getState() == EXEC);
    // cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    ORANGE_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    }
    catch(std::exception& e) {
        cur->m_state = EXCPT;
        ORANGE_LOG_ERROR(g_logger) << "Fiber Excpt: " << e.what();
    }
    catch(...) {
        cur->m_state = EXCPT;
        ORANGE_LOG_ERROR(g_logger) << "Fiber Excpt";
    }
    Fiber* fiber = cur.get();
    cur.reset();
    fiber->swapOut();
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    ORANGE_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch(std::exception& e) {
        cur->m_state = EXCPT;
        ORANGE_LOG_ERROR(g_logger) << "Fiber Excpt: " << e.what();
    } catch(...) {
        cur->m_state = EXCPT;
        ORANGE_LOG_ERROR(g_logger) << "Fiber Excpt";
    }

    Fiber* fiber = cur.get();
    cur.reset();
    fiber->back();
}

}
