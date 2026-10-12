// ps3sh — consola para hablar con el servidor ps3link del PS3.
//
//   ps3sh                     abre la consola interactiva
//   ps3sh info                ejecuta un comando y sale
//   ps3sh -H 127.0.0.1 ping   contra otro host (p. ej. ps3mock)
//
// Host por defecto: $PS3_HOST o 10.0.0.2. Puerto: $PS3_PORT o 4747.
#include "client.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace ps3link;

namespace {

struct Session {
    std::string host;
    uint16_t port;
    Client client;

    Client& conn() {
        if (!client.connected()) client.connect(host, port);
        return client;
    }
};

const char* kHelp =
    "Comandos:\n"
    "  ping [n]                      latencia ida y vuelta (n veces, por defecto 4)\n"
    "  info                          información del servidor\n"
    "  echo <texto>                  el servidor devuelve el texto\n"
    "  mandel [WxH] [iter] [archivo] fractal calculado en el servidor; guarda .pgm si se indica\n"
    "  bench [WxH] [iter]            mismo fractal en el servidor y en la laptop, compara tiempos\n"
    "  shutdown                      cierra la app del PS3 (vuelve al XMB)\n"
    "  help | exit\n";

bool parse_size(const std::string& s, uint16_t& w, uint16_t& h) {
    unsigned a = 0, b = 0;
    if (std::sscanf(s.c_str(), "%ux%u", &a, &b) != 2 || a == 0 || b == 0 || a > 65535 || b > 65535) return false;
    w = uint16_t(a);
    h = uint16_t(b);
    return true;
}

MandelRequest mandel_args(const std::vector<std::string>& args, size_t first) {
    MandelRequest r;
    r.width = 640;
    r.height = 480;
    r.max_iter = 256;
    r.x0 = -2.5;
    r.y0 = -1.25;
    r.x1 = 1.0;
    r.y1 = 1.25;
    if (args.size() > first && !parse_size(args[first], r.width, r.height))
        throw std::invalid_argument("tamaño inválido, usa WxH (p. ej. 640x480)");
    if (args.size() > first + 1) {
        const long it = std::strtol(args[first + 1].c_str(), nullptr, 10);
        if (it < 1 || it > 65535) throw std::invalid_argument("iter debe estar entre 1 y 65535");
        r.max_iter = uint16_t(it);
    }
    return r;
}

void write_pgm(const std::string& path, const MandelRequest& r, const std::vector<uint8_t>& px) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("no se pudo escribir " + path);
    f << "P5\n" << r.width << " " << r.height << "\n255\n";
    f.write(reinterpret_cast<const char*>(px.data()), std::streamsize(px.size()));
}

double ms(uint64_t us) { return double(us) / 1000.0; }

// Devuelve false si hay que salir.
bool run_command(Session& s, const std::vector<std::string>& args) {
    if (args.empty()) return true;
    const std::string& cmd = args[0];

    if (cmd == "exit" || cmd == "quit") return false;
    if (cmd == "help" || cmd == "?") {
        std::cout << kHelp;
    } else if (cmd == "ping") {
        const int n = args.size() > 1 ? std::max(1, std::atoi(args[1].c_str())) : 4;
        for (int i = 0; i < n; ++i)
            std::printf("pong de %s: %.3f ms\n", s.host.c_str(), ms(uint64_t(s.conn().ping().count())));
    } else if (cmd == "info") {
        std::cout << s.conn().info() << "\n";
    } else if (cmd == "echo") {
        std::string text;
        for (size_t i = 1; i < args.size(); ++i) text += (i > 1 ? " " : "") + args[i];
        const auto out = s.conn().echo(std::vector<uint8_t>(text.begin(), text.end()));
        std::cout << std::string(out.begin(), out.end()) << "\n";
    } else if (cmd == "mandel") {
        const MandelRequest r = mandel_args(args, 1);
        const auto t0 = std::chrono::steady_clock::now();
        const MandelResult res = s.conn().mandel(r);
        const auto total = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0);
        std::printf("%ux%u, %u iter: cómputo %.1f ms, total con red %.1f ms\n", r.width, r.height, r.max_iter,
                    ms(res.elapsed_us), ms(uint64_t(total.count())));
        if (args.size() > 3) {
            write_pgm(args[3], r, res.pixels);
            std::cout << "guardado en " << args[3] << "\n";
        }
    } else if (cmd == "bench") {
        const MandelRequest r = mandel_args(args, 1);
        const MandelResult remote = s.conn().mandel(r);
        std::vector<uint8_t> local(size_t(r.width) * r.height);
        const auto t0 = std::chrono::steady_clock::now();
        compute_mandel(r, local.data());
        const auto local_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - t0);
        std::printf("servidor: %.1f ms | laptop (1 hilo): %.1f ms | resultados %s\n", ms(remote.elapsed_us),
                    ms(uint64_t(local_us.count())), remote.pixels == local ? "idénticos" : "DISTINTOS");
    } else if (cmd == "shutdown") {
        s.conn().shutdown();
        std::cout << "servidor cerrado\n";
    } else {
        std::cout << "comando desconocido: " << cmd << " (escribe help)\n";
    }
    return true;
}

std::vector<std::string> split(const std::string& line) {
    std::istringstream in(line);
    std::vector<std::string> out;
    for (std::string w; in >> w;) out.push_back(w);
    return out;
}

// Ejecuta un comando reportando errores. Devuelve false si hay que salir.
bool run_safely(Session& s, const std::vector<std::string>& args, bool& failed) {
    try {
        return run_command(s, args);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        failed = true;
        return true;
    }
}

}  // namespace

int main(int argc, char** argv) {
    Session s;
    s.host = std::getenv("PS3_HOST") ? std::getenv("PS3_HOST") : "10.0.0.2";
    s.port = std::getenv("PS3_PORT") ? uint16_t(std::atoi(std::getenv("PS3_PORT"))) : kDefaultPort;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if ((a == "-H" || a == "--host") && i + 1 < argc) {
            s.host = argv[++i];
        } else if ((a == "-p" || a == "--port") && i + 1 < argc) {
            s.port = uint16_t(std::atoi(argv[++i]));
        } else if (a == "-h" || a == "--help") {
            std::cout << "uso: ps3sh [-H host] [-p puerto] [comando...]\n\n" << kHelp;
            return 0;
        } else {
            args.push_back(a);
        }
    }

    bool failed = false;
    if (!args.empty()) {
        run_safely(s, args, failed);
        return failed ? 1 : 0;
    }

    std::cout << "ps3sh → " << s.host << ":" << s.port << " (escribe help)\n";
    std::string line;
    while (std::cout << "ps3> " << std::flush, std::getline(std::cin, line)) {
        if (!run_safely(s, split(line), failed)) break;
    }
    return 0;
}
