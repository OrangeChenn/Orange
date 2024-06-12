#include "src/env.h"

int main(int argc, char** argv) {
    orange::EnvMrg::GetInstance()->addHelp("s", "start with the terminal");
    orange::EnvMrg::GetInstance()->addHelp("d", "run as daemon");
    orange::EnvMrg::GetInstance()->addHelp("p", "print help");
    if(!orange::EnvMrg::GetInstance()->init(argc, argv)) {
        orange::EnvMrg::GetInstance()->printHelp();
        return 0;
    }

    if(orange::EnvMrg::GetInstance()->has("p")) {
        orange::EnvMrg::GetInstance()->printHelp();
    }

    return 0;
}
