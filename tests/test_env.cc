#include "src/env.h"
#include <iostream>

int main(int argc, char** argv) {
    orange::EnvMrg::GetInstance()->addHelp("s", "start with the terminal");
    orange::EnvMrg::GetInstance()->addHelp("d", "run as daemon");
    orange::EnvMrg::GetInstance()->addHelp("p", "print help");
    if(!orange::EnvMrg::GetInstance()->init(argc, argv)) {
        orange::EnvMrg::GetInstance()->printHelp();
        return 0;
    }

    std::cout << "exe=" << orange::EnvMrg::GetInstance()->getEve()
             << "\ncwd=" << orange::EnvMrg::GetInstance()->getCwd() << std::endl;

    std::cout << "path=" << orange::EnvMrg::GetInstance()->getEnv("PATH", "xxx") << std::endl;
    std::cout << "test=" << orange::EnvMrg::GetInstance()->getEnv("TEST", "xxx") << std::endl;
    std::cout << "set env test " << orange::EnvMrg::GetInstance()->setEnv("TEST", "yyy") << std::endl;
    std::cout << "test=" << orange::EnvMrg::GetInstance()->getEnv("TEST", "xxx") << std::endl;

    if(orange::EnvMrg::GetInstance()->has("p")) {
        orange::EnvMrg::GetInstance()->printHelp();
    }

    return 0;
}
