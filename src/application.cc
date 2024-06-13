#include "application.h"

#include "address.h"
#include "config.h"
#include "daemon.h"
#include "env.h"
#include "log.h"
#include "iomanager.h"

namespace orange {

static orange::Logger::ptr g_logger = ORANGE_LOG_NAME("system");

static orange::ConfigVar<std::string>::ptr g_server_work_path =
        orange::Config::Lookup<std::string>("server.work_path"
        , std::string("/apps/work/orange"), "server  work path");

static orange::ConfigVar<std::string>::ptr g_server_pid_file =
        orange::Config::Lookup<std::string>("server.pid_file"
        , std::string("orange.pid"), "server pid file");

struct HttpServerConf {
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 2 * 60 * 1000;
    std::string name;

    bool operator==(const HttpServerConf& rhs) const {
        return this->address == rhs.address
            && this->keepalive == rhs.keepalive
            && this->timeout == rhs.timeout
            && this->name == rhs.name;
    }

    bool isValid() const {
        return !address.empty();
    }
};

template<>
class LexicalCast<std::string, HttpServerConf> {
public:
    HttpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template<>
class LexicalCast<HttpServerConf, std::string> {
public:
    std::string operator()(const HttpServerConf& conf) {
        YAML::Node node;
        for(size_t i = 0; i < conf.address.size(); ++i) {
            node["address"].push_back(conf.address[i]);
        }
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["name"] = conf.name;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

static orange::ConfigVar<std::vector<HttpServerConf>>::ptr g_http_server_conf =
        orange::Config::Lookup<std::vector<HttpServerConf>>("http_servers"
        , std::vector<HttpServerConf>(), "http server config");

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

bool Application::init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;
    orange::EnvMrg::GetInstance()->addHelp("s", "start with the terminal");
    orange::EnvMrg::GetInstance()->addHelp("d", "run as daemon");
    orange::EnvMrg::GetInstance()->addHelp("c", "conf path default: ./conf");
    orange::EnvMrg::GetInstance()->addHelp("p", "print help");
    
    if(!orange::EnvMrg::GetInstance()->init(argc, argv)) {
        orange::EnvMrg::GetInstance()->printHelp();
        return false;
    }

    if(orange::EnvMrg::GetInstance()->has("p")) {
        orange::EnvMrg::GetInstance()->printHelp();
        return false;
    }

    int run_type = 0;
    if(orange::EnvMrg::GetInstance()->has("s")) {
        run_type = 1;
    }
    if(orange::EnvMrg::GetInstance()->has("d")) {
        run_type = 2;
    }
    if(run_type == 0) {
        orange::EnvMrg::GetInstance()->printHelp();
        return false;
    }

    std::string pidfile = g_server_work_path->getValue() + "/" 
            + g_server_pid_file->getValue();
    if(orange::FSUtil::IsRunningPidfile(pidfile)) {
        ORANGE_LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }
    
    std::string conf_path = orange::EnvMrg::GetInstance()->getAbsolutePath(
            orange::EnvMrg::GetInstance()->get("c", "conf")
            );
    ORANGE_LOG_INFO(g_logger) << "load conf path:" << conf_path;
    orange::Config::LoadFromConfDir(conf_path);

    if(!orange::FSUtil::Mkdir(g_server_work_path->getValue())) {
        ORANGE_LOG_ERROR(g_logger) << "create work path [" << g_server_work_path->getValue()
                << " error=" << errno << " strerror=" << strerror(errno);
        return false;
    }
    return true;
}

bool Application::run() {
    bool is_daemon = orange::EnvMrg::GetInstance()->has("d");
    return orange::start_daemon(m_argc, m_argv
            , std::bind(&Application::main, this, std::placeholders::_1
            , std::placeholders::_2), is_daemon);
}

int Application::main(int argc, char** argv) {
    std::string pidfile = g_server_work_path->getValue() + "/" 
            + g_server_pid_file->getValue();
    std::ofstream ofs(pidfile);
    if(!ofs) {
        ORANGE_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
        return false;
    }
    ofs << getpid();
    ofs.flush();

    orange::IOManager iom(1);
    iom.schedule(std::bind(&Application::run_fiber, this));
    return 0;
}

int Application::run_fiber() {
    auto http_conf = g_http_server_conf->getValue();
    for(auto i : http_conf) {
        ORANGE_LOG_INFO(g_logger) << LexicalCast<HttpServerConf, std::string>()(i);

        std::vector<orange::Address::ptr> address;
        for(auto a : i.address) {
            size_t pos = a.find(":");
            if(pos == std::string::npos) {
                ORANGE_LOG_ERROR(g_logger) << "invalid address: " << a;
                continue;
            }
            auto addr = orange::Address::LookupAny(a);
            if(addr) {
                address.push_back(addr);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t>> result;
            if(!orange::Address::GetInterfaceAddress(result, a.substr(0, pos))) {
                ORANGE_LOG_ERROR(g_logger) << "invalid address: " << a;
                continue;
            }
            for(auto& x : result) {
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                if(ipaddr) {
                    ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                }
                address.push_back(ipaddr);
            }
        }
        orange::http::HttpServer::ptr server(new orange::http::HttpServer(i.keepalive));
        std::vector<orange::Address::ptr> fails;
        if(!server->bind(address, fails)) {
            for(auto& x : fails) {
                ORANGE_LOG_ERROR(g_logger) << "bind address fail:" << *x;
            }
            _exit(0);
        }
        if(!i.name.empty()) {
            server->setName(i.name);
        }
        server->start();
        m_httpservers.push_back(server);
    }
    return 0;
}

} // namespace orange
