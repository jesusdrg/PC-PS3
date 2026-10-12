# Entorno de desarrollo (CachyOS)

## 1. Herramientas de la laptop

```sh
sudo pacman -S --needed base-devel cmake ninja curl
```

Compilar y probar el cliente sin el PS3:

```sh
cmake -S host -B host/build -G Ninja
cmake --build host/build
ctest --test-dir host/build        # pruebas, incluye cliente ↔ ps3mock
./host/build/ps3mock &             # servidor falso en 127.0.0.1:4747
./host/build/ps3sh -H 127.0.0.1    # consola
```

## 2. Toolchain del PS3 (ps3toolchain + PSL1GHT)

Compila GCC, binutils y newlib para la PPU y las SPUs, más PSL1GHT. Tarda entre
30 y 90 minutos y se hace una sola vez.

```sh
sudo pacman -S --needed autoconf automake bison flex texinfo ncurses zlib \
    gmp mpfr libmpc libelf python wget patch pkgconf openssl libtool bzip2

# en ~/.bashrc o ~/.zshrc (CachyOS usa fish por defecto: ver abajo)
export PS3DEV=/opt/ps3dev
export PSL1GHT=$PS3DEV
export PATH=$PATH:$PS3DEV/bin:$PS3DEV/ppu/bin:$PS3DEV/spu/bin

sudo mkdir -p $PS3DEV && sudo chown $USER $PS3DEV
git clone https://github.com/ps3dev/ps3toolchain.git
cd ps3toolchain
./toolchain.sh 1 2 4 5 6 8   # sin gdb (3, 7) ni ps3libraries (9) por ahora
```

Si falla alguna descarga:

- **GCC SPU no baja GMP/MPFR** (usa FTP): en `build/gcc-9.5.0/contrib/download_prerequisites`
  cambien `ftp://` por `https://` en `base_url`, borren los `.tar.*` vacíos de esa
  carpeta, ejecuten el script y repitan `./toolchain.sh 6 8`.
- **PSL1GHT no baja el tarball:** clónenlo a mano y en esa carpeta ejecuten
  `make install-ctrl && make && make install`.

En fish:

```fish
set -Ux PS3DEV /opt/ps3dev
set -Ux PSL1GHT $PS3DEV
fish_add_path $PS3DEV/bin $PS3DEV/ppu/bin $PS3DEV/spu/bin
```

## 3. Compilar e instalar la app en el PS3

```sh
cd ps3
make pkg            # genera ps3link.pkg
```

**Primera vez:** copia `ps3link.pkg` al PS3 (USB en FAT32 o FTP de webMAN a
`/dev_hdd0/packages/`) e instálalo desde *Juego → Package Manager → Install
Package Files*. Aparecerá el ícono **ps3link** en *Juego*.

**Después de cada cambio** (con HEN activo y la red configurada, ver `red.md`):

```sh
make deploy         # recompila y sube solo EBOOT.BIN por FTP
```

Luego abran el ícono desde el XMB. La pantalla se queda en negro: es normal,
el servidor no dibuja nada todavía. Desde la laptop:

```sh
./host/build/ps3sh info
./host/build/ps3sh bench 640x480 256
```

Para salir: `ps3sh shutdown` o el botón PS → Salir del juego.
