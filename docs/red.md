# Conectar la laptop y el PS3 con un cable

Cable de red directo laptop ↔ PS3. No hace falta cable cruzado: la tarjeta de la
T480 (Intel I219) detecta la polaridad sola (auto-MDIX).

| Equipo | IP | Máscara |
|---|---|---|
| Laptop | `10.0.0.1` | `255.255.255.0` |
| PS3 | `10.0.0.2` | `255.255.255.0` |

## Laptop (CachyOS, NetworkManager)

Averigua el nombre de la interfaz cableada; en la T480 suele ser `enp0s31f6`:

```sh
ip -br link
```

Crea un perfil fijo para el PS3. Así no toca tu Wi-Fi, que sigue dando internet:

```sh
nmcli con add type ethernet ifname enp0s31f6 con-name ps3 \
    ipv4.method manual ipv4.addresses 10.0.0.1/24 ipv4.never-default yes \
    ipv6.method disabled
nmcli con up ps3
```

`ipv4.never-default yes` evita que el tráfico a internet intente salir por el cable.

## PS3

**Ajustes → Ajustes de red → Ajustes de conexión a Internet → Personalizado**

| Opción | Valor |
|---|---|
| Método de conexión | Por cable |
| Modo de funcionamiento del dispositivo de red | Detección automática |
| Ajustes de dirección IP | Manual |
| Dirección IP | `10.0.0.2` |
| Máscara de subred | `255.255.255.0` |
| Router predeterminado | `10.0.0.1` |
| DNS primario | `10.0.0.1` |
| DNS secundario | (vacío) |
| MTU | Automático |
| Servidor proxy | No usar |
| UPnP | Desactivar |

La prueba de conexión dirá que no hay internet. Es normal; lo que importa es que
consiga la IP.

## Comprobar

Desde la laptop, con HEN activado en el PS3:

```sh
ping -c 3 10.0.0.2
```

Si tienen webMAN MOD, abran `http://10.0.0.2` en el navegador de la laptop:
deberían ver su panel.
