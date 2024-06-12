#include "../src/log.h"
#include "../src/config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include "../src/env.h"


#if 0
orange::ConfigVar<int>::ptr g_int_value_config = 
    orange::Config::Lookup("system.port", (int)8080, "system port");

orange::ConfigVar<float>::ptr g_int_valuex_config = 
    orange::Config::Lookup("system.port", (float)8080, "system port");

orange::ConfigVar<float>::ptr g_float_value_config = 
    orange::Config::Lookup("system.value", (float)10.4, "system value");

orange::ConfigVar<std::vector<int>>::ptr g_int_vector_value_config = 
    orange::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int_vec value");

orange::ConfigVar<std::list<int>>::ptr g_int_list_value_config = 
    orange::Config::Lookup("system.int_list", std::list<int>{1, 2}, "system int_list value");

orange::ConfigVar<std::set<int>>::ptr g_int_set_value_config = 
    orange::Config::Lookup("system.int_set", std::set<int>{1, 2}, "system int_set value");

orange::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config = 
    orange::Config::Lookup("system.int_uset", std::unordered_set<int>{1, 2}, "system int_uset value");

orange::ConfigVar<std::map<std::string, int>>::ptr g_int_map_value_config = 
    orange::Config::Lookup("system.int_map", std::map<std::string, int>{{"k", 2}}, "system int_map value");

orange::ConfigVar<std::unordered_map<std::string, int>>::ptr g_int_umap_value_config = 
    orange::Config::Lookup("system.int_umap", std::unordered_map<std::string, int>{{"k", 2}}, "system int_umap value");

void print_yaml(const YAML::Node& node, int level) {
    if(node.IsScalar()) {
        ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << std::string(level * 4, ' ')
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if(node.IsNull()) {
        ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << std::string(level * 4, ' ')
            << "Null - " << node.Type() << level;
    } else if(node.IsMap()) {
        for(auto it = node.begin(); it != node.end(); ++it) {
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << std::string(level * 4, ' ')
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if(node.IsSequence()) {
        for(size_t i = 0; i < node.size(); ++i) {
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << std::string(level * 4, ' ')
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node node = YAML::LoadFile("/home/chen/Code/C++/Orange/bin/conf/test.yaml");
    print_yaml(node, 0);
    // ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << node;
}

void test_config() {
    // ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "before: " << g_int_valuex_config->getValue();
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "before: " << g_float_value_config->toString();

    #define XX(g_val, name, prefix) \
    { \
        auto& v = g_val->getValue(); \
        for(auto& i : v) { \
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << #prefix ": " #name " " << i; \
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << #prefix ": " #name " " << "Yaml: " << g_val->toString(); \
        } \
    }

    #define XX_M(g_val, name, prefix) \
    { \
        auto& v = g_val->getValue(); \
        for(auto& i : v) { \
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << #prefix ": " #name ":{ " << i.first \
                << " - " << i.second << " }"; \
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << #prefix ": " #name " " << "Yaml: " << g_val->toString(); \
        } \
    }

    XX(g_int_vector_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_int_map_value_config, int_map, before);
    XX_M(g_int_umap_value_config, int_umap, before);

    YAML::Node node = YAML::LoadFile("/home/chen/Code/C++/Orange/bin/conf/test.yaml");
    orange::Config::LoadFromYaml(node);

    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "after: " << g_float_value_config->toString();

    XX(g_int_vector_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, agter);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_int_map_value_config, int_map, after);
    XX_M(g_int_umap_value_config, int_umap, after);
}

#endif

class Person {
public:
    std::string m_name = "";
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex;
        return ss.str();
    }

    bool operator==(const Person& rhs) const {
        return this->m_name == rhs.m_name
            && this->m_age == rhs.m_age
            && this->m_sex == rhs.m_sex;
    }
};

namespace orange {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

} // namespace orange

orange::ConfigVar<Person>::ptr g_person_config = 
    orange::Config::Lookup("class.person", Person{}, "class person");

orange::ConfigVar<std::vector<Person>>::ptr g_person_vec = 
    orange::Config::Lookup("class.vec", std::vector<Person>{}, "class person vec");    

orange::ConfigVar<std::map<std::string, Person>>::ptr g_person_map = 
    orange::Config::Lookup("class.map", std::map<std::string, Person>{}, "class person map");

orange::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr g_person_map_vec = 
    orange::Config::Lookup("class.map_vec", std::map<std::string, std::vector<Person>>{}, "class person map");

void test_class() {
    #define XX_PM(g_val, prefix) \
    { \
        auto& m = g_val->getValue(); \
        for(const auto& i : m) { \
            ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << #prefix << ": " <<  i.first \
                << " - " << i.second.toString();\
        } \
        ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << #prefix << ": size = " << m.size(); \
    }

    g_person_config->addListener([](const Person& old_val, const Person& new_val) {
        ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "old_val= " << old_val.toString()
            << " new_val= " << new_val.toString();
    });

    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "before: " << g_person_config->getValue().toString() << " - " << g_person_config->toString();
    XX_PM(g_person_map, before);
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "before: " << g_person_map_vec->toString();
    YAML::Node root = YAML::LoadFile("/home/chen/Code/C++/Orange/bin/conf/test.yaml");
    orange::Config::LoadFromYaml(root);
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "after: " << g_person_config->getValue().toString() << " - " << g_person_config->toString();
    XX_PM(g_person_map, after);
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "after: " << g_person_vec->toString();
    ORANGE_LOG_INFO(ORANGE_LOG_ROOT()) << "after: " << g_person_map_vec->toString();
}

void test_log() {
    static orange::Logger::ptr system_log = ORANGE_LOG_NAME("system");
    ORANGE_LOG_INFO(system_log) << "hello system";
    std::cout << orange::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/chen/Code/C++/Orange/bin/conf/log.yaml");
    orange::Config::LoadFromYaml(root);

    std::cout << "---------------------------" << std::endl;
    std::cout << orange::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    ORANGE_LOG_INFO(system_log) << "hello system";
    system_log->setFormatter("%d - %m%n");
    ORANGE_LOG_INFO(system_log) << "hello system";
}

void test_load_conf() {
    orange::Config::LoadFromConfDir("conf");
}

int main(int argc, char** argv) {
    // test_yaml();
    // test_config();
    // test_class();
    // test_log();
    orange::EnvMrg::GetInstance()->init(argc, argv);
    test_load_conf();
    std::cout << "==============================" << std::endl;
    sleep(20);
    test_load_conf();
    return 0;
}
