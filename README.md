# PC-PS3

Usar un PS3 (HFW + HEN) como coprocesador de una laptop (T480, CachyOS),
conectados con un cable de red.

La meta: que el Cell del PS3 (PPU + 6 SPUs) haga la simulación pesada —agua,
física, partículas— y que la laptop se dedique solo a dibujar con Vulkan. Es
la misma arquitectura que hacía ver bien a los juegos de PS3, solo que
repartida entre dos máquinas.

```
 Laptop (CachyOS)                         PS3 (HFW + HEN)
 ┌──────────────────┐   Ethernet directo  ┌───────────────────────────┐
 │ ps3sh / renderer │◄──── TCP :4747 ────►│ ps3link (homebrew PSL1GHT)│
 │ C++ / Vulkan     │                     │  PPU: red + despachador   │
 └──────────────────┘                     │  SPU 0..5: jobs           │
                                          └───────────────────────────┘
```

## Estructura

| Ruta | Qué es |
|---|---|
| `common/ps3link/` | Protocolo compartido (header-only C++). Lo compilan ambos lados. |
| `host/` | Lado laptop: `ps3sh` (consola), `ps3mock` (PS3 simulado), pruebas. |
| `ps3/` | Lado PS3: servidor homebrew (PSL1GHT). |
| `docs/protocolo.md` | Formato de los mensajes. |
| `docs/red.md` | Cómo conectar laptop y PS3 con el cable. |
| `docs/desarrollo.md` | Toolchain en CachyOS, compilar, instalar y desplegar. |

## Arranque rápido (sin PS3)

```sh
cmake -S host -B host/build -G Ninja && cmake --build host/build
ctest --test-dir host/build
./host/build/ps3mock &
./host/build/ps3sh -H 127.0.0.1
ps3> info
ps3> bench 640x480 256
```

## Plan

1. **Tubería** ← *aquí estamos*: servidor en el PS3, consola `ps3sh`, PING / INFO / ECHO y un benchmark en la PPU.
2. **Jobs en SPU:** la laptop manda un programa SPU + datos y recibe el resultado.
3. **Renderer Vulkan** en la T480: océano, cielo e iluminación, simulados localmente.
4. **Híbrido:** la simulación del océano pasa a las SPUs y llega por red. Se mide tiempo por frame, temperatura y frecuencia de la GPU, comparando contra el modo local.
