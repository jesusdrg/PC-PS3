// ps3link — servidor que corre en el PS3 (homebrew PSL1GHT, requiere HEN o CFW).
//
// Hilo principal: atiende eventos del sistema (salir con el botón PS).
// Hilo de red: acepta conexiones TCP en el puerto 4747 y las sirve con el
// mismo dispatch que usa ps3mock en la laptop.
#include <ps3link/serve.hpp>

#include <arpa/inet.h>
#include <net/net.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <lv2/systime.h>
#include <sys/thread.h>
#include <sysutil/sysutil.h>
#include <unistd.h>

#include <cstdio>

using namespace ps3link;

static volatile bool g_running = true;
static int g_listen_fd = -1;

static void on_sysutil(u64 status, u64 /*param*/, void* /*user*/) {
    if (status == SYSUTIL_EXIT_GAME) g_running = false;
}

static uint64_t now_us() { return uint64_t(sysGetSystemTime()); }

static void server_thread(void* /*arg*/) {
    ServerContext ctx;
    char info[128];
    std::snprintf(info, sizeof info, "{\"server\":\"ps3link\",\"protocol\":%u,\"platform\":\"ps3-ppu\"}",
                  unsigned(kVersion));
    ctx.info_json = info;
    ctx.now_us = now_us;

    while (g_running && !ctx.shutdown_requested) {
        const int fd = accept(g_listen_fd, nullptr, nullptr);
        if (fd < 0) {
            usleep(100 * 1000);
            continue;
        }
        serve_connection(fd, ctx);
        close(fd);
    }
    g_running = false;
    sysThreadExit(0);
}

static int open_listen_socket(uint16_t port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) != 0 || listen(fd, 4) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main() {
    netInitialize();
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, on_sysutil, nullptr);

    g_listen_fd = open_listen_socket(kDefaultPort);
    if (g_listen_fd < 0) {
        std::printf("ps3link: no se pudo abrir el puerto %u\n", unsigned(kDefaultPort));
        netDeinitialize();
        return 1;
    }
    std::printf("ps3link: escuchando en el puerto %u\n", unsigned(kDefaultPort));

    sys_ppu_thread_t tid;
    sysThreadCreate(&tid, server_thread, nullptr, 1000, 256 * 1024, THREAD_JOINABLE,
                    const_cast<char*>("ps3link-net"));

    // El hilo de red termina con SHUTDOWN; el botón PS llega por sysutil.
    while (g_running) {
        sysUtilCheckCallback();
        usleep(100 * 1000);
    }

    close(g_listen_fd);
    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    netDeinitialize();
    return 0;
}
