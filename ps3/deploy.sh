#!/usr/bin/env bash
# Sube un EBOOT.BIN nuevo a la app ps3link ya instalada en el PS3, usando el
# servidor FTP de webMAN MOD. Luego se abre el ícono desde el XMB.
#
#   PS3_HOST=10.0.0.2 APPID=PS3L00001 ./deploy.sh build/pkg/USRDIR/EBOOT.BIN
set -euo pipefail

EBOOT="${1:?uso: deploy.sh ruta/a/EBOOT.BIN}"
PS3_HOST="${PS3_HOST:-10.0.0.2}"
APPID="${APPID:-PS3L00001}"
DEST="ftp://${PS3_HOST}/dev_hdd0/game/${APPID}/USRDIR/EBOOT.BIN"

echo "subiendo $(basename "$EBOOT") → $DEST"
curl --fail --silent --show-error --connect-timeout 5 -T "$EBOOT" "$DEST"
echo "listo: abre ps3link desde el XMB (Juego)"
