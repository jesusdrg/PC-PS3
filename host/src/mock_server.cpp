#include "mock_server.hpp"

#include <ps3link/serve.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace ps3link {

namespace {

uint64_t steady_us() {
    using namespace std::chrono;
    return uint64_t(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

std::string mock_info() {
    return std::string("{\"server\":\"ps3mock\",\"protocol\":") + std::to_string(kVersion) +
           ",\"platform\":\"host\",\"threads\":" + std::to_string(std::thread::hardware_concurrency()) + "}";
}

}  // namespace

MockServer::~MockServer() {
    if (listen_fd_ >= 0) ::close(listen_fd_);
}

uint16_t MockServer::listen(const std::string& bind_addr, uint16_t port) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
    int one = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, bind_addr.c_str(), &addr.sin_addr) != 1)
        throw std::runtime_error("dirección inválida: " + bind_addr);
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0)
        throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
    if (::listen(listen_fd_, 4) != 0) throw std::runtime_error(std::string("listen: ") + std::strerror(errno));

    socklen_t len = sizeof addr;
    getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
    return ntohs(addr.sin_port);
}

void MockServer::run() {
    ServerContext ctx;
    ctx.info_json = mock_info();
    ctx.now_us = steady_us;
    while (!stopping_ && !ctx.shutdown_requested) {
        const int fd = accept(listen_fd_, nullptr, nullptr);
        if (fd < 0) {
            if (errno == EINTR) continue;
            break;  // stop() cerró el socket
        }
        int one = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
        serve_connection(fd, ctx);
        ::close(fd);
    }
}

void MockServer::stop() {
    stopping_ = true;
    if (listen_fd_ >= 0) ::shutdown(listen_fd_, SHUT_RDWR);
}

}  // namespace ps3link
