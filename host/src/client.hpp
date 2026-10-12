#pragma once

#include <ps3link/protocol.hpp>

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

namespace ps3link {

struct NetError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Error devuelto por el servidor (status != 0).
struct RemoteError : std::runtime_error {
    uint16_t status;
    RemoteError(uint16_t s, const std::string& msg) : std::runtime_error(msg), status(s) {}
};

struct MandelResult {
    uint64_t elapsed_us = 0;  // tiempo de cómputo medido en el servidor
    std::vector<uint8_t> pixels;
};

class Client {
public:
    Client() = default;
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void connect(const std::string& host, uint16_t port, int timeout_ms = 3000);
    void close();
    bool connected() const { return fd_ >= 0; }

    // Envía una petición y espera su respuesta. Lanza RemoteError si status != 0.
    std::vector<uint8_t> request(uint8_t type, const std::vector<uint8_t>& payload = {});

    std::chrono::microseconds ping();
    std::string info();
    std::vector<uint8_t> echo(const std::vector<uint8_t>& data);
    MandelResult mandel(const MandelRequest& req);
    void shutdown();

private:
    int fd_ = -1;
    uint32_t next_id_ = 1;
};

}  // namespace ps3link
