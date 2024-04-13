#pragma once

#include <pthread.h>

#include <functional>
#include <memory>

#include "mutex.h"
#include "noncopyable.h"

namespace orange {

class Thread : Noncopyable {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();
    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    void join();

    static void* run(void* arg);

    static Thread* GetThis();
    static std::string GetName();
    static void SetName(const std::string& name);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;

    orange::Semaphore m_semaphore;
};

} // namespace orange
