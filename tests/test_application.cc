#include "src/application.h"

int main(int argc, char** argv) {
    orange::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}
