#include "client.hpp"

#include <ps3link/serve.hpp>

#include <cerrno>
#include <cstring>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace ps3link {

Client::~Client() { close(); }

void Client::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

void Client::connect(const std::string& host, uint16_t port, int timeout_ms) {
    close();
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    const std::string port_str = std::to_string(port);
    if (int rc = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res); rc != 0)
        throw NetError("no se pudo resolver " + host + ": " + gai_strerror(rc));

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        throw NetError(std::string("socket: ") + std::strerror(errno));
    }
    // El timeout aplica a connect (SO_SNDTIMEO) y a cada recv/send.
    timeval tv{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);

    const int rc = ::connect(fd, res->ai_addr, res->ai_addrlen);
    const int err = errno;
    freeaddrinfo(res);
    if (rc != 0) {
        ::close(fd);
        throw NetError("no se pudo conectar a " + host + ":" + port_str + ": " + std::strerror(err));
    }
    fd_ = fd;
}

std::vector<uint8_t> Client::request(uint8_t type, const std::vector<uint8_t>& payload) {
    if (fd_ < 0) throw NetError("no hay conexión");
    Header h;
    h.type = type;
    h.req_id = next_id_++;
    h.length = uint32_t(payload.size());
    if (!send_message(fd_, h, payload.data())) {
        close();
        throw NetError("error al enviar (¿se cerró la conexión?)");
    }

    Header resp;
    std::vector<uint8_t> body;
    if (!recv_message(fd_, resp, body)) {
        close();
        throw NetError("error al recibir: conexión cerrada, timeout o cabecera inválida");
    }
    if (resp.type != uint8_t(type | kResponseBit) || resp.req_id != h.req_id) {
        close();
        throw NetError("respuesta inesperada del servidor");
    }
    if (resp.status != ST_OK)
        throw RemoteError(resp.status, status_name(resp.status) + ": " + std::string(body.begin(), body.end()));
    return body;
}

std::chrono::microseconds Client::ping() {
    const auto t0 = std::chrono::steady_clock::now();
    request(MSG_PING);
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0);
}

std::string Client::info() {
    const auto body = request(MSG_INFO);
    return std::string(body.begin(), body.end());
}

std::vector<uint8_t> Client::echo(const std::vector<uint8_t>& data) { return request(MSG_ECHO, data); }

MandelResult Client::mandel(const MandelRequest& req) {
    const auto body = request(MSG_MANDEL, encode_mandel(req));
    const size_t pixels = size_t(req.width) * req.height;
    if (body.size() != 8 + pixels) throw NetError("MANDEL: tamaño de respuesta incorrecto");
    MandelResult r;
    r.elapsed_us = get_u64(body.data());
    r.pixels.assign(body.begin() + 8, body.end());
    return r;
}

void Client::shutdown() {
    request(MSG_SHUTDOWN);
    close();
}

}  // namespace ps3link
