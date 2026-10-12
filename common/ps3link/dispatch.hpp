// ps3link — lógica de servidor compartida: recibe una petición ya decodificada
// y produce la respuesta. La usan tanto el servidor real del PS3 como el
// servidor simulado de la laptop (ps3mock), así el mock se comporta igual.
#pragma once

#include "protocol.hpp"

namespace ps3link {

struct ServerContext {
    std::string info_json;              // respuesta de INFO, la arma cada plataforma
    uint64_t (*now_us)() = nullptr;     // reloj monotónico en microsegundos
    bool shutdown_requested = false;    // lo activa SHUTDOWN
};

struct Response {
    uint16_t status = ST_OK;
    std::vector<uint8_t> payload;
};

inline Response error_response(uint16_t status, const char* msg) {
    Response r;
    r.status = status;
    r.payload.assign(msg, msg + std::strlen(msg));
    return r;
}

inline Response dispatch(ServerContext& ctx, uint8_t type, const std::vector<uint8_t>& payload) {
    Response r;
    switch (type) {
        case MSG_PING:
            return r;

        case MSG_INFO:
            r.payload.assign(ctx.info_json.begin(), ctx.info_json.end());
            return r;

        case MSG_ECHO:
            r.payload = payload;
            return r;

        case MSG_MANDEL: {
            MandelRequest req;
            if (!decode_mandel(payload.data(), uint32_t(payload.size()), req))
                return error_response(ST_BAD_PAYLOAD, "MANDEL: parametros invalidos");
            const uint32_t pixels = uint32_t(req.width) * req.height;
            r.payload.resize(8 + pixels);
            const uint64_t t0 = ctx.now_us ? ctx.now_us() : 0;
            compute_mandel(req, &r.payload[8]);
            const uint64_t t1 = ctx.now_us ? ctx.now_us() : 0;
            put_u64(&r.payload[0], t1 - t0);
            return r;
        }

        case MSG_SHUTDOWN:
            ctx.shutdown_requested = true;
            return r;

        default:
            return error_response(ST_UNKNOWN_TYPE, "tipo de mensaje desconocido");
    }
}

}  // namespace ps3link
