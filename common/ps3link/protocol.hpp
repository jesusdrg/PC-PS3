// ps3link — protocolo compartido entre la laptop y el PS3.
// Ver docs/protocolo.md. Header-only, C++11, sin dependencias de plataforma:
// la (de)serialización usa desplazamientos de bytes, así funciona igual en
// x86 (little-endian) y en el Cell (big-endian).
#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace ps3link {

const uint32_t kMagic = 0x5053334C;  // "PS3L"
const uint8_t kVersion = 1;
const uint16_t kDefaultPort = 4747;
const uint32_t kHeaderSize = 16;
const uint32_t kMaxPayload = 16u * 1024u * 1024u;
const uint8_t kResponseBit = 0x80;

enum MsgType : uint8_t {
    MSG_PING = 0x01,
    MSG_INFO = 0x02,
    MSG_ECHO = 0x03,
    MSG_MANDEL = 0x10,
    MSG_SHUTDOWN = 0x7F,
};

enum Status : uint16_t {
    ST_OK = 0,
    ST_UNKNOWN_TYPE = 1,
    ST_BAD_PAYLOAD = 2,
    ST_INTERNAL = 3,
};

// --- primitivas big-endian -------------------------------------------------

inline void put_u16(uint8_t* p, uint16_t v) {
    p[0] = uint8_t(v >> 8);
    p[1] = uint8_t(v);
}
inline void put_u32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v >> 24);
    p[1] = uint8_t(v >> 16);
    p[2] = uint8_t(v >> 8);
    p[3] = uint8_t(v);
}
inline void put_u64(uint8_t* p, uint64_t v) {
    put_u32(p, uint32_t(v >> 32));
    put_u32(p + 4, uint32_t(v));
}
inline void put_f64(uint8_t* p, double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof bits);
    put_u64(p, bits);
}

inline uint16_t get_u16(const uint8_t* p) { return uint16_t((p[0] << 8) | p[1]); }
inline uint32_t get_u32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
inline uint64_t get_u64(const uint8_t* p) { return (uint64_t(get_u32(p)) << 32) | get_u32(p + 4); }
inline double get_f64(const uint8_t* p) {
    uint64_t bits = get_u64(p);
    double v;
    std::memcpy(&v, &bits, sizeof v);
    return v;
}

// --- cabecera --------------------------------------------------------------

struct Header {
    uint8_t type = 0;
    uint16_t status = ST_OK;
    uint32_t req_id = 0;
    uint32_t length = 0;
};

inline void encode_header(const Header& h, uint8_t out[kHeaderSize]) {
    put_u32(out, kMagic);
    out[4] = kVersion;
    out[5] = h.type;
    put_u16(out + 6, h.status);
    put_u32(out + 8, h.req_id);
    put_u32(out + 12, h.length);
}

// Devuelve false si la cabecera no es válida (magic, versión o longitud).
inline bool decode_header(const uint8_t in[kHeaderSize], Header& h) {
    if (get_u32(in) != kMagic || in[4] != kVersion) return false;
    h.type = in[5];
    h.status = get_u16(in + 6);
    h.req_id = get_u32(in + 8);
    h.length = get_u32(in + 12);
    return h.length <= kMaxPayload;
}

// --- MANDEL ----------------------------------------------------------------

const uint32_t kMandelReqSize = 40;
const uint32_t kMandelMaxPixels = 4u * 1024u * 1024u;

struct MandelRequest {
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t max_iter = 0;
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
};

inline std::vector<uint8_t> encode_mandel(const MandelRequest& r) {
    std::vector<uint8_t> b(kMandelReqSize, 0);
    put_u16(&b[0], r.width);
    put_u16(&b[2], r.height);
    put_u16(&b[4], r.max_iter);
    put_u16(&b[6], 0);
    put_f64(&b[8], r.x0);
    put_f64(&b[16], r.y0);
    put_f64(&b[24], r.x1);
    put_f64(&b[32], r.y1);
    return b;
}

inline bool decode_mandel(const uint8_t* p, uint32_t len, MandelRequest& r) {
    if (len != kMandelReqSize) return false;
    r.width = get_u16(p);
    r.height = get_u16(p + 2);
    r.max_iter = get_u16(p + 4);
    r.x0 = get_f64(p + 8);
    r.y0 = get_f64(p + 16);
    r.x1 = get_f64(p + 24);
    r.y1 = get_f64(p + 32);
    uint32_t pixels = uint32_t(r.width) * r.height;
    return pixels > 0 && pixels <= kMandelMaxPixels && r.max_iter >= 1;
}

// Calcula el fractal en `out` (width*height bytes). Mismo código en ambos lados
// para que la comparación de tiempos sea justa.
inline void compute_mandel(const MandelRequest& r, uint8_t* out) {
    const double dx = (r.x1 - r.x0) / r.width;
    const double dy = (r.y1 - r.y0) / r.height;
    for (uint32_t py = 0; py < r.height; ++py) {
        const double ci = r.y0 + dy * (py + 0.5);
        for (uint32_t px = 0; px < r.width; ++px) {
            const double cr = r.x0 + dx * (px + 0.5);
            double zr = 0, zi = 0;
            uint32_t it = 0;
            while (it < r.max_iter && zr * zr + zi * zi <= 4.0) {
                const double t = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = t;
                ++it;
            }
            out[py * r.width + px] = uint8_t(it * 255u / r.max_iter);
        }
    }
}

inline std::string status_name(uint16_t s) {
    switch (s) {
        case ST_OK: return "ok";
        case ST_UNKNOWN_TYPE: return "tipo desconocido";
        case ST_BAD_PAYLOAD: return "payload inválido";
        case ST_INTERNAL: return "error interno";
        default: return "status " + std::to_string(s);
    }
}

}  // namespace ps3link
