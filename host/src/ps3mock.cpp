// ps3mock — servidor ps3link que corre en la laptop, para desarrollar sin el PS3.
//
//   ps3mock [-b 127.0.0.1] [-p 4747]
#include "mock_server.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string bind_addr = "127.0.0.1";
    uint16_t port = ps3link::kDefaultPort;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-b" && i + 1 < argc) {
            bind_addr = argv[++i];
        } else if (a == "-p" && i + 1 < argc) {
            port = uint16_t(std::atoi(argv[++i]));
        } else {
            std::cerr << "uso: ps3mock [-b dirección] [-p puerto]\n";
            return 2;
        }
    }
    try {
        ps3link::MockServer server;
        port = server.listen(bind_addr, port);
        std::cout << "ps3mock escuchando en " << bind_addr << ":" << port << "\n";
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
