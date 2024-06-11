#include "daemon.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "log.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");
static orange::ConfigVar<uint32_t>::ptr g_daemon_restart_interval
        = orange::Config::Lookup<uint32_t>("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

std::string ProcessInfo::toString() const {
    std::stringstream ss;
    ss << "[ProcessInfo parent_id=" << parent_id
       << " main_id=" << main_id
       << " parent_start_time=" << Time2Str(parent_start_time)
       << " main_start_time=" << Time2Str(main_start_time)
       << " restart_count=" << restart_count;
    return ss.str();
}

static int real_start(int argc, char** argv
                    , std::function<int(int, char**)> main_cb) {
    return main_cb(argc, argv);
}

static int real_daemon(int argc, char** argv
                    , std::function<int(int, char**)> main_cb) {
    ProcessInfoMrg::GetInstance()->parent_id = getpid();
    ProcessInfoMrg::GetInstance()->parent_start_time = time(0);
    while(true) {
        pid_t pid = fork();
        if(pid == 0) {
            ProcessInfoMrg::GetInstance()->main_id = getpid();
            ProcessInfoMrg::GetInstance()->main_start_time = time(0);
            ORANGE_LOG_INFO(g_logger) << "process start pid=" << pid;
            return real_start(argc, argv, main_cb);
        } else if(pid < 0) {
            ORANGE_LOG_ERROR(g_logger) << "fork fail return pid=" << pid;
            return -1;
        } else {
            int status = 0;
            waitpid(pid, &status, 0);
            if(status) {
                ORANGE_LOG_ERROR(g_logger) << "child crash pid=" << pid;
            } else {
                ORANGE_LOG_INFO(g_logger) << "child finished pid=" << pid;
                break;
            }
            ProcessInfoMrg::GetInstance()->restart_count += 1;
            sleep(g_daemon_restart_interval->getValue());
        }
    }
    return 0;
}

int start_daemon(int argc, char** argv
                , std::function<int(int, char**)> main_cb
                , bool is_daemon) {
    if(!is_daemon) {
        return real_start(argc, argv, main_cb);
    }
    return real_daemon(argc, argv, main_cb);
}

}
