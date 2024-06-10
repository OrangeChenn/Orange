#include "uri.h"

int main(int argc, char** argv) {
    orange::Uri::ptr uri = orange::Uri::Create("http://www.baudi.com/test/中文/uri?id=1&name=chen#fragment");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}
