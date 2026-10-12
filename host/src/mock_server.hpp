// Servidor ps3link que corre en la laptop. Usa el mismo dispatch que el PS3,
// así se puede desarrollar y probar el cliente sin tener la consola conectada.
#pragma once

#include <ps3link/dispatch.hpp>

#include <atomic>
#include <string>

namespace ps3link {

class MockServer {
public:
    ~MockServer();

    // Abre el socket de escucha. port = 0 elige uno libre. Devuelve el puerto real.
    uint16_t listen(const std::string& bind_addr, uint16_t port);
    // Atiende conexiones (una a la vez) hasta SHUTDOWN o stop(). Bloquea.
    void run();
    // Seguro de llamar desde otro hilo.
    void stop();

private:
    int listen_fd_ = -1;
    std::atomic<bool> stopping_{false};
};

}  // namespace ps3link
