// ps3link — bucle de una conexión (leer petición → dispatch → responder).
// Usa sockets BSD, disponibles tanto en Linux como en PSL1GHT.
#pragma once

#include "dispatch.hpp"

#include <sys/socket.h>
#include <sys/types.h>

namespace ps3link {

inline bool recv_all(int fd, uint8_t* buf, size_t len) {
    while (len > 0) {
        const ssize_t n = recv(fd, buf, len, 0);
        if (n <= 0) return false;
        buf += n;
        len -= size_t(n);
    }
    return true;
}

inline bool send_all(int fd, const uint8_t* buf, size_t len) {
    while (len > 0) {
        const ssize_t n = send(fd, buf, len, 0);
        if (n <= 0) return false;
        buf += n;
        len -= size_t(n);
    }
    return true;
}

inline bool send_message(int fd, const Header& h, const uint8_t* payload) {
    uint8_t hdr[kHeaderSize];
    encode_header(h, hdr);
    if (!send_all(fd, hdr, kHeaderSize)) return false;
    return h.length == 0 || send_all(fd, payload, h.length);
}

// Lee un mensaje completo. Devuelve false si se cerró la conexión o la
// cabecera es inválida (en ese caso hay que cerrar: el stream quedó desalineado).
inline bool recv_message(int fd, Header& h, std::vector<uint8_t>& payload) {
    uint8_t hdr[kHeaderSize];
    if (!recv_all(fd, hdr, kHeaderSize) || !decode_header(hdr, h)) return false;
    payload.resize(h.length);
    return h.length == 0 || recv_all(fd, payload.data(), h.length);
}

// Atiende peticiones hasta que el cliente cierre o llegue SHUTDOWN.
inline void serve_connection(int fd, ServerContext& ctx) {
    Header req;
    std::vector<uint8_t> payload;
    while (!ctx.shutdown_requested && recv_message(fd, req, payload)) {
        Response r = dispatch(ctx, req.type, payload);
        Header h;
        h.type = uint8_t(req.type | kResponseBit);
        h.status = r.status;
        h.req_id = req.req_id;
        h.length = uint32_t(r.payload.size());
        if (!send_message(fd, h, r.payload.data())) return;
    }
}

}  // namespace ps3link
