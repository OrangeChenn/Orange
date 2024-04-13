#include <unistd.h>

#include "../src/thread.h"
#include "../src/mutex.h"
#include "../src/log.h"
#include "../src/util.h"
#include <vector>

orange::Logger::ptr g_logger = ORANGE_LOG_ROOT();

int count = 0;
// orange::RWMutex mutex;
orange::Mutex mutex;

void func1() {
    ORANGE_LOG_INFO(g_logger) << "name: " << orange::Thread::GetName()
                             << " this.name: " << orange::Thread::GetThis()->getName()
                             << " id: " << orange::GetThreadId()
                             << " this.id: " << orange::Thread::GetThis()->getId();
    // sleep(20);
    for(int i = 0; i < 100000; ++i) {
        // orange::RWMutex::WriteLock lock(mutex);
        orange::Mutex::Lock lock(mutex);
        ++count;
    }
}

void func2() {

}

int main(int argc, char** argv) {
    std::vector<orange::Thread::ptr> thrs;
    for(int i = 0; i < 5; ++i) {
        orange::Thread::ptr thr(new orange::Thread(func1, "name-" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }

    ORANGE_LOG_INFO(g_logger) << "thread end";
    ORANGE_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}
