#pragma once

#include "http/http_server.h"

namespace orange {

class Application {
public:
    Application();

    bool init(int argc, char** argv);
    bool run();
private:
    int main(int argc, char** argv);
    int run_fiber();
private:
    int m_argc;
    char** m_argv;
    static Application* s_instance;
    std::vector<orange::http::HttpServer::ptr> m_httpservers;
};

} // namespace orange
