#include "env.h"

#include <string.h>
#include <iomanip>
#include <iostream>
#include "log.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

bool Env::init(int argc, char** argv) {
    m_program = argv[0];
    const char* now_key = nullptr;
    for(int i = 1; i < argc; ++i) {
        if(argv[i][0] == '-') {
            if(strlen(argv[i]) > 1) {
                if(now_key) {
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
            } else {
                ORANGE_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                        << " val=" << argv[i];
                return false;
            }
        } else {
            if(now_key) {
                add(now_key, argv[i]);
                now_key = nullptr;
            } else {
                ORANGE_LOG_ERROR(g_logger) << "invalid arg idx=" << i
                        << " val=" << argv[i];
                return false;
            }
        }
    }
    if(now_key) {
        add(now_key, "");
    }
    return true;
}

void Env::add(const std::string& key, const std::string& val) {
    RWMutexType::WriteLock lock(m_mutex);
    m_args[key] = val;
}

void Env::del(const std::string& key) {
    RWMutexType::WriteLock lock(m_mutex);
    m_args.erase(key);
}

bool Env::has(const std::string& key) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}

std::string Env::get(const std::string& key, const std::string& default_val) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end() ? m_args[key] : default_val;
}

void Env::addHelp(const std::string& key, const std::string& desc) {
    removeHelp(key);
    RWMutexType::WriteLock lock(m_mutex);
    m_helps.push_back(std::make_pair(key, desc));
}

void Env::removeHelp(const std::string& key) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_helps.begin(); it != m_helps.end();) {
        if(it->first == key) {
            m_helps.erase(it);
        } else {
            ++it;
        }
    }
}

void Env::printHelp() {
    RWMutexType::ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_program << " [options]" << std::endl;
    for(auto& i : m_helps) {
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}

} // namespace orange
