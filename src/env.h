#pragma once

#include <map>
#include <string>
#include <vector>

#include "mutex.h"
#include "singleton.h"

namespace orange {

class Env {
public:
    typedef std::shared_ptr<Env> ptr;
    typedef orange::RWMutex RWMutexType;

    bool init(int argc, char** argv);

    void add(const std::string& key, const std::string& val);
    void del(const std::string& key);
    bool has(const std::string& key);
    std::string get(const std::string& key, const std::string& default_val = "");

    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp();

    const std::string& getEve() const { return m_exe; }
    const std::string& getCwd() const { return m_cwd; }

    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& default_val = "");

    std::string getAbsolutePath(const std::string& path) const;
private:
    RWMutexType m_mutex;
    std::map<std::string, std::string> m_args;
    std::vector<std::pair<std::string, std::string>> m_helps;
    std::string m_program;
    std::string m_exe;
    std::string m_cwd;
};

typedef orange::Singleton<Env> EnvMrg;

} // namespace orange
