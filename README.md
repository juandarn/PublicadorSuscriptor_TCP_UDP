# 🧩 Laboratorio 3 – Sistema Publicador/Suscriptor (TCP y UDP)

Este laboratorio implementa un sistema **Publicador–Suscriptor** en **C**, utilizando **sockets TCP y UDP** sobre **Winsock2 (Windows)**.
El objetivo es comparar el comportamiento de ambos protocolos de transporte —**confiable (TCP)** y **no confiable (UDP)**— en un entorno controlado.

El sistema simula el envío de **noticias deportivas** en tiempo real, donde:

* Los **publicadores** envían eventos (goles, tarjetas, cambios, etc.).
* Los **suscriptores** se registran por tema (por ejemplo, *PartidoA*, *PartidoB*).
* El **broker** (servidor) recibe los mensajes y los distribuye a los suscriptores correspondientes.

---

## ⚙️ Tecnologías utilizadas

* Lenguaje: **C (C11)**
* Librería de red: **Winsock2**
* Compilador: **GCC (MinGW-w64 o MSYS2)**
* Entorno: **Visual Studio Code / PowerShell / MSYS2**
---

## 📁 Estructura del proyecto

```
PublicadorSuscriptor_TCP_UDP/
│
├── tcp/                       # Implementación con TCP
│   ├── broker_tcp.c
│   ├── publisher_tcp.c
│   ├── subscriber_tcp.c
│   ├── tcp_utils.c
│   ├── tcp_utils.h
│   ├── Makefile
│   └── output/
│
├── udp/                       # Implementación con UDP
│   ├── broker_udp.c
│   ├── publisher_udp.c
│   ├── subscriber_udp.c
│   ├── udp_utils.c
│   ├── udp_utils.h
│   └── output/
└── README.md                  
```

---

## 🧱 Compilación (Windows + GCC)

### 🔹 Requisitos previos

1. Instala **MSYS2** → [https://www.msys2.org](https://www.msys2.org)
2. Abre la terminal **MSYS2 UCRT64**
3. Instala las herramientas:

   ```bash
   pacman -Syu
   pacman -S --needed mingw-w64-ucrt-x86_64-toolchain
   ```
4. Verifica:

   ```bash
   gcc --version
   ```

---

### 🔹 Compilar TCP

Desde la carpeta `tcp`:

```powershell
mkdir output 2>$null

gcc broker_tcp.c tcp_utils.c -o output/broker_tcp.exe -lws2_32
gcc publisher_tcp.c tcp_utils.c -o output/publisher_tcp.exe -lws2_32
gcc subscriber_tcp.c tcp_utils.c -o output/subscriber_tcp.exe -lws2_32
```

---

### 🔹 Compilar UDP

Desde la carpeta `udp`:

```powershell
mkdir output 2>$null

gcc broker_udp.c udp_utils.c -o output/broker_udp.exe -lws2_32
gcc publisher_udp.c udp_utils.c -o output/publisher_udp.exe -lws2_32
gcc subscriber_udp.c udp_utils.c -o output/subscriber_udp.exe -lws2_32
```

> En ambos casos, la opción `-lws2_32` enlaza la librería **Winsock2** necesaria para sockets en Windows.

---

## 🚀 Ejecución

### 🧩 TCP

1️⃣ **Broker (servidor)**

```powershell
.\tcp\output\broker_tcp.exe
```

2️⃣ **Suscriptores**

```powershell
.\tcp\output\subscriber_tcp.exe 127.0.0.1 PartidoA
.\tcp\output\subscriber_tcp.exe 127.0.0.1 PartidoB
```

3️⃣ **Publicadores**

```powershell
.\tcp\output\publisher_tcp.exe 127.0.0.1 PartidoA "Gol EquipoA minuto 32"
.\tcp\output\publisher_tcp.exe 127.0.0.1 PartidoB "Tarjeta amarilla #10"
```

---

### 📡 UDP

1️⃣ **Broker**

```powershell
.\udp\output\broker_udp.exe
```

2️⃣ **Suscriptores**

```powershell
.\udp\output\subscriber_udp.exe 127.0.0.1 PartidoA
.\udp\output\subscriber_udp.exe 127.0.0.1 PartidoB
```

3️⃣ **Publicadores**

```powershell
.\udp\output\publisher_udp.exe 127.0.0.1 PartidoA "Gol EquipoA min32"
.\udp\output\publisher_udp.exe 127.0.0.1 PartidoB "Tarjeta amarilla #10"
```

> En UDP se usa el puerto **8081**, mientras que TCP usa el **8080**.

---

## 🧪 Pruebas con Wireshark

1. Abre **Wireshark** y selecciona tu interfaz de red local.
2. Usa filtros:

   * Para TCP: `tcp.port == 8080`
   * Para UDP: `udp.port == 8081`
3. Guarda las capturas:

   ```
   captures/tcp_pubsub.pcap
   captures/udp_pubsub.pcap
   ```
4. Analiza los resultados para cada protocolo:

   * Pérdida de paquetes
   * Orden de llegada
   * Overhead (tamaño de encabezados)
   * Retransmisiones

---

## 📊 Comparación entre TCP y UDP

| Característica       | **TCP**                                  | **UDP**                  |
| -------------------- | ---------------------------------------- | ------------------------ |
| Conexión             | Orientado a conexión                     | No orientado a conexión  |
| Confiabilidad        | Alta, retransmite                        | No garantiza entrega     |
| Orden                | Garantizado                              | No garantizado           |
| Overhead             | Mayor                                    | Menor                    |
| Retransmisión        | Sí                                       | No                       |
| Aplicaciones típicas | Mensajería, control, streaming confiable | Juegos, multimedia, VoIP |
| Puerto usado         | 8080                                     | 8081                     |

---

## 🧠 Conclusiones esperadas

* TCP ofrece confiabilidad y orden, pero con mayor latencia y overhead.
* UDP es más rápido, pero puede generar pérdidas o desorden en la entrega.
* La elección depende del tipo de aplicación (control vs tiempo real).

---

## ⚠️ Notas importantes

* Si aparece una alerta de firewall, permite el acceso de red **privada/local**.
* Usa **Ctrl + C** para detener los procesos.
* Puedes ejecutar todos los programas en **localhost** o en distintas máquinas de una LAN.
* Si algún ejecutable no arranca, revisa que `WSAStartup()` no esté bloqueado por antivirus.

---

## ✨ Créditos

Desarrollado por **Juan David Ríos**
Curso: *Infraestructura de Comunicaciones*
**Departamento de Ingeniería de Sistemas y Computación** – Universidad de los Andes 🇨🇴
Profesor Asistente: **Nathalia Quiroga**

