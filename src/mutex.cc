#include "mutex.h"

#include <stdexcept>

namespace orange {

Semaphore::Semaphore(const uint32_t count /* = 0 */) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() noexcept(false) {
    if(sem_destroy(&m_semaphore)) {
        throw std::logic_error("sem_destroy error");
    }
}

void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}
    
}
