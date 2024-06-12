#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <list>

#include "env.h"
#include "util.h"

namespace orange {

// Config::ConfigVarMap Config::s_datas;
static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : GetDatas()[name];
}

static void ListAllMember(const std::string& prefix,
                          const YAML::Node& node,
                          std::list<std::pair<std::string, const YAML::Node> >& output) {
    if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890") != std::string::npos) {
        ORANGE_LOG_ERROR(ORANGE_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if(node.IsMap()) {
        for(auto it = node.begin(); it != node.end(); ++it) {
            ListAllMember(prefix.empty() ? it->first.Scalar() :
                          prefix + "." + it->first.Scalar(), it->second, output);
        }
    }
}

void Config::LoadFromYaml(const YAML::Node& node) {
    std::list<std::pair<std::string, const YAML::Node> > all_nodes;
    ListAllMember("", node, all_nodes);

    for(auto& i : all_nodes) {
        std::string key = i.first;
        if(key.empty()) {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBase::ptr val = LookupBase(key);
        if(val) {
            if(i.second.IsScalar()) {
                val->fromString(i.second.Scalar());
            } else {
                std::stringstream ss;
                ss << i.second;
                val->fromString(ss.str());
            }
        }
    }
}

static std::map<std::string, uint64_t> s_file2modifytime;
static orange::Mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path) {
    std::string absolute_path = orange::EnvMrg::GetInstance()->getAbsolutePath(path);
    std::vector<std::string> files;
    orange::FSUtil::ListAllFiles(files, absolute_path, ".yml");

    for(auto& i : files) {
        {
            struct stat st;
            if(lstat(i.c_str(), &st) == 0) {
                orange::Mutex::Lock lock(s_mutex);
                if(s_file2modifytime[i] == (uint64_t)st.st_mtime) {
                    continue;
                }
                s_file2modifytime[i] = (uint64_t)st.st_mtime;
            }
        }
        try {
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            ORANGE_LOG_INFO(g_logger) << "LoadConfFile file="
                    << i << " ok";
        } catch(...) {
            ORANGE_LOG_INFO(g_logger) << "LoadConfFile file="
                    << i << " fail";
        }
    }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
    RWMutexType::ReadLock lock(GetMutex());
    ConfigVarMap& m = GetDatas();
    for(auto it = m.begin();
            it !=m.end(); ++it) {
        cb(it->second);
    }
}

}
