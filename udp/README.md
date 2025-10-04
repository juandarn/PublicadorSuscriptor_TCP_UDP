# 📡 Laboratorio 3 – Sistema Publicador/Suscriptor UDP

Este proyecto implementa el modelo **Publicador–Suscriptor** en **C (Winsock2 / UDP)**, simulando un sistema de **noticias deportivas no confiable** (sin conexión persistente).
A diferencia del caso TCP, **UDP no garantiza entrega ni orden**, lo que permite analizar el comportamiento del protocolo frente a pérdidas o duplicación de mensajes.

---

## 📁 Estructura de carpetas

```
PublicadorSuscriptor_TCP_UDP/
│
├── udp/
│    ├── broker_udp.c
│    ├── publisher_udp.c
│    ├── subscriber_udp.c
│    ├── udp_utils.c
│    ├── udp_utils.h
│    └── output/                # Carpeta de salida
```

---

## ⚙️ Requisitos

* **Windows 10/11**
* **GCC (MinGW-w64 o MSYS2 UCRT64)**
* **VS Code** con la extensión *C/C++*
  *(Opcional)*: extensión *Code Runner* o *Tasks* para compilar con un clic.

> Verifica que `gcc` esté disponible:
>
> ```bash
> gcc --version
> ```

---

## 🧱 Compilación (Windows + gcc)

Desde la carpeta `udp`, ejecuta en la terminal:

```powershell
# crea la carpeta de salida (opcional)
mkdir output 2>$null

# compila cada binario incluyendo udp_utils.c y enlazando la librería de sockets de Windows
gcc broker_udp.c udp_utils.c -o output/broker_udp.exe -lws2_32
gcc publisher_udp.c udp_utils.c -o output/publisher_udp.exe -lws2_32
gcc subscriber_udp.c udp_utils.c -o output/subscriber_udp.exe -lws2_32
```

> 🔹 Se usa el puerto **8081** (definido en `BROKER_UDP_PORT`) para no interferir con el TCP (8080).

---

## 🚀 Ejecución del sistema UDP

Abre **tres o más terminales** en la carpeta `udp/`.

### 1️⃣ Broker (servidor central)

```powershell
.\output\broker_udp.exe
```

> Queda escuchando datagramas en el puerto **8081**.

---

### 2️⃣ Suscriptores (clientes que se registran por tema)

```powershell
.\output\subscriber_udp.exe 127.0.0.1 PartidoA
.\output\subscriber_udp.exe 127.0.0.1 PartidoB
```

Cada suscriptor envía un datagrama `SUB <topic>` al broker para registrarse.

---

### 3️⃣ Publicadores (emiten mensajes sobre un tema)

```powershell
.\output\publisher_udp.exe 127.0.0.1 PartidoA "Gol EquipoA minuto 32"
.\output\publisher_udp.exe 127.0.0.1 PartidoB "Tarjeta amarilla #10"
```

El broker recibe cada mensaje `PUB` y lo reenvía como
`MSG <topic> <payload>` a todos los suscriptores registrados con ese tema.

---

## 🧪 Pruebas con Wireshark

1. Inicia **Wireshark** y captura en la interfaz de red local.
2. Filtro recomendado:

   ```bash
   udp.port == 8081
   ```
3. Guarda la captura como:

   ```
   captures/udp_pubsub.pcap
   ```
4. Analiza las diferencias frente a TCP en cuanto a:

   * Retransmisiones (no existen en UDP)
   * Pérdidas de mensajes
   * Orden de entrega
   * Overhead y tamaño de los encabezados

---

## 📊 Comparación TCP vs UDP

| Criterio             | TCP                                      | UDP                                          |
| -------------------- | ---------------------------------------- | -------------------------------------------- |
| Confiabilidad        | Alta (entrega garantizada)               | Baja (puede perder paquetes)                 |
| Orden de entrega     | Garantizado                              | No garantizado                               |
| Retransmisiones      | Sí                                       | No                                           |
| Control de flujo     | Sí                                       | No                                           |
| Overhead             | Mayor                                    | Menor                                        |
| Aplicaciones típicas | Mensajería, control, streaming confiable | Juegos, multimedia, voz/video en tiempo real |

---

## ⚠️ Notas importantes

* Si el firewall bloquea el broker, permite el acceso a red privada.
* Los datagramas pueden perderse o llegar fuera de orden: **es el comportamiento esperado** para el análisis.
* Usa `Ctrl+C` para finalizar cada proceso.
* El mismo código funciona en Linux cambiando `winsock2.h` por `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>` y removiendo `WSAStartup()`/`WSACleanup()`.

---

## ✨ Créditos

Desarrollado en C para el curso **Infraestructura de Comunicaciones** –
**Departamento de Ingeniería de Sistemas y Computación, Universidad de los Andes**
Profesor Asistente: *Nathalia Quiroga*

