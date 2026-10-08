# Protocolo ps3link v1

TCP, puerto por defecto **4747**. Todo en **big-endian** (orden de red, y además
nativo del Cell/PPU, así que el PS3 no tiene que hacer byte-swap).

## Cabecera (16 bytes)

| offset | tamaño | campo     | notas                                   |
|-------:|-------:|-----------|-----------------------------------------|
| 0      | 4      | magic     | `PS3L`                                  |
| 4      | 1      | version   | `1`                                     |
| 5      | 1      | type      | tipo de mensaje; respuesta = `type \| 0x80` |
| 6      | 2      | status    | 0 = ok; ≠0 = error (payload = mensaje UTF-8) |
| 8      | 4      | req_id    | lo elige el cliente, el servidor lo devuelve igual |
| 12     | 4      | length    | bytes de payload que siguen (máx. 16 MiB) |

## Mensajes

| type | nombre    | payload petición | payload respuesta |
|-----:|-----------|------------------|-------------------|
| 0x01 | PING      | vacío            | vacío             |
| 0x02 | INFO      | vacío            | JSON UTF-8        |
| 0x03 | ECHO      | bytes            | los mismos bytes  |
| 0x10 | MANDEL    | ver abajo        | ver abajo         |
| 0x7F | SHUTDOWN  | vacío            | vacío; el servidor cierra y la app vuelve al XMB |

### MANDEL (0x10)

Benchmark de cómputo; sirve para comparar PPU vs. laptop y, en la fase 2, SPUs.

Petición (40 bytes):

```
u16 width, u16 height, u16 max_iter, u16 reservado(0),
f64 x0, f64 y0, f64 x1, f64 y1
```

Respuesta: `u64 elapsed_us` + `width*height` bytes, un `u8` por píxel
(`iter * 255 / max_iter`), fila por fila.

Límites: `width*height <= 4 194 304`, `1 <= max_iter <= 65535`.

## Códigos de status

| status | significado |
|-------:|-------------|
| 0 | ok |
| 1 | tipo desconocido |
| 2 | payload inválido |
| 3 | error interno |
