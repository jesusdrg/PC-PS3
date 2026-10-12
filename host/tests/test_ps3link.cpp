// Pruebas: codificación del protocolo + cliente contra ps3mock (de punta a punta).
#include "client.hpp"
#include "mock_server.hpp"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <thread>

using namespace ps3link;

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::fprintf(stderr, "%s:%d: falló: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

static void test_header_roundtrip() {
    Header h;
    h.type = MSG_MANDEL | kResponseBit;
    h.status = ST_BAD_PAYLOAD;
    h.req_id = 0xDEADBEEF;
    h.length = 1234;
    uint8_t buf[kHeaderSize];
    encode_header(h, buf);
    // Big-endian en el cable, sin importar la arquitectura.
    CHECK(buf[0] == 'P' && buf[1] == 'S' && buf[2] == '3' && buf[3] == 'L');
    CHECK(buf[8] == 0xDE && buf[11] == 0xEF);

    Header d;
    CHECK(decode_header(buf, d));
    CHECK(d.type == h.type && d.status == h.status && d.req_id == h.req_id && d.length == h.length);

    buf[0] = 'X';
    CHECK(!decode_header(buf, d));
    encode_header(h, buf);
    put_u32(buf + 12, kMaxPayload + 1);
    CHECK(!decode_header(buf, d));
}

static void test_mandel_codec() {
    MandelRequest r;
    r.width = 320;
    r.height = 200;
    r.max_iter = 100;
    r.x0 = -2.5;
    r.y0 = -1.0;
    r.x1 = 1.0;
    r.y1 = 1.0;
    const auto b = encode_mandel(r);
    CHECK(b.size() == kMandelReqSize);
    MandelRequest d;
    CHECK(decode_mandel(b.data(), uint32_t(b.size()), d));
    CHECK(d.width == 320 && d.height == 200 && d.max_iter == 100 && d.x0 == -2.5 && d.y1 == 1.0);

    r.max_iter = 0;
    const auto bad = encode_mandel(r);
    CHECK(!decode_mandel(bad.data(), uint32_t(bad.size()), d));
}

static void test_end_to_end() {
    MockServer server;
    const uint16_t port = server.listen("127.0.0.1", 0);
    std::thread th([&] { server.run(); });

    Client c;
    c.connect("127.0.0.1", port);
    c.ping();
    CHECK(c.info().find("\"ps3mock\"") != std::string::npos);

    const std::vector<uint8_t> data = {0, 1, 2, 250, 255};
    CHECK(c.echo(data) == data);
    CHECK(c.echo({}).empty());

    MandelRequest r;
    r.width = 64;
    r.height = 48;
    r.max_iter = 50;
    r.x0 = -2.0;
    r.y0 = -1.0;
    r.x1 = 1.0;
    r.y1 = 1.0;
    const MandelResult res = c.mandel(r);
    std::vector<uint8_t> local(64 * 48);
    compute_mandel(r, local.data());
    CHECK(res.pixels == local);

    bool threw = false;
    try {
        c.request(0x55);
    } catch (const RemoteError& e) {
        threw = e.status == ST_UNKNOWN_TYPE;
    }
    CHECK(threw);
    CHECK(c.connected());  // un error remoto no rompe la conexión

    c.shutdown();
    th.join();
}

int main() {
    test_header_roundtrip();
    test_mandel_codec();
    test_end_to_end();
    if (g_failures) {
        std::fprintf(stderr, "%d prueba(s) fallaron\n", g_failures);
        return 1;
    }
    std::puts("todas las pruebas pasaron");
    return 0;
}
