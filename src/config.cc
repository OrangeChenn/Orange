#include "config.h"

#include <list>

namespace orange {

// Config::ConfigVarMap Config::s_datas;

ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
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

}
