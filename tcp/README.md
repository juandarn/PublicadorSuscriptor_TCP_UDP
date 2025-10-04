# 📰 Laboratorio 3 – Sistema Publicador/Suscriptor TCP

Este proyecto implementa un modelo **Publicador–Suscriptor** en **C (Winsock2 / TCP)**, simulando un sistema de **noticias deportivas en tiempo real**.
Incluye tres componentes principales:

* **Broker:** actúa como servidor y distribuye los mensajes a los suscriptores correctos.
* **Publisher:** publica mensajes (eventos deportivos) sobre un tema específico.
* **Subscriber:** se suscribe a un tema y recibe los mensajes en tiempo real.

---

## 📁 Estructura de carpetas

```
PublicadorSuscriptor_TCP_UDP/
│
├── tcp/
│   ├── broker_tcp.c
│   ├── publisher_tcp.c
│   ├── subscriber_tcp.c
│   ├── tcp_utils.c
│   ├── tcp_utils.h
│   └── output/                # Carpeta de salida 
```

---

## ⚙️ Requisitos

* **Windows 10/11**
* **GCC (MinGW-w64 o MSYS2 UCRT64)**
* **VS Code** con extensión *C/C++*
  *(Opcional)*: extensión *Code Runner* o *Tasks* para compilar con un clic.

> Verifica que `gcc` esté disponible:
>
> ```bash
> gcc --version
> ```

---

## 🧱 Compilación (Windows + gcc)

Desde la carpeta `tcp`, ejecuta en la terminal (PowerShell o MSYS2):

```powershell
# crea la carpeta de salida (opcional)
mkdir output 2>$null

# compila cada binario incluyendo tcp_utils.c y enlazando -lws2_32
gcc broker_tcp.c tcp_utils.c -o output/broker_tcp.exe -lws2_32
gcc publisher_tcp.c tcp_utils.c -o output/publisher_tcp.exe -lws2_32
gcc subscriber_tcp.c tcp_utils.c -o output/subscriber_tcp.exe -lws2_32
```

---

## 🚀 Ejecución

Abre **tres o más terminales** en la carpeta `tcp/`.

### 1️⃣ Broker (servidor central)

```powershell
.\output\broker_tcp.exe
```

> Queda escuchando en el puerto **8080**.

---

### 2️⃣ Suscriptores (hinchas)

```powershell
.\output\subscriber_tcp.exe 127.0.0.1 PartidoA
.\output\subscriber_tcp.exe 127.0.0.1 PartidoB
```

Cada suscriptor elige el “tema” (partido) al que se suscribe.

---

### 3️⃣ Publicadores (periodistas)

```powershell
.\output\publisher_tcp.exe 127.0.0.1 PartidoA "Gol EquipoA minuto 32"
.\output\publisher_tcp.exe 127.0.0.1 PartidoA "Cambio entra #10 sale #20"
.\output\publisher_tcp.exe 127.0.0.1 PartidoB "Tarjeta amarilla #10"
```

Los suscriptores recibirán solo los mensajes del tema al que están suscritos.

---

## 🧪 Pruebas con Wireshark

1. Inicia **Wireshark** y captura en la interfaz de red local.
2. Filtro recomendado:

   * Para TCP: `tcp.port == 8080`
3. Guarda el archivo de captura como:

   ```
   captures/tcp_pubsub.pcap
   ```
4. Analiza confiabilidad, orden, pérdidas y overhead según el informe del laboratorio.

---

## 📊 Análisis requerido

En el informe compara los resultados entre **TCP** y (posteriormente) **UDP**:

| Criterio             | TCP                                      | UDP (esperado)                          |
| -------------------- | ---------------------------------------- | --------------------------------------- |
| Confiabilidad        | Alta, entrega garantizada                | No garantizada                          |
| Orden de entrega     | Garantizado                              | Puede variar                            |
| Pérdida de mensajes  | Muy baja                                 | Posible                                 |
| Overhead             | Mayor                                    | Menor                                   |
| Aplicaciones típicas | Mensajería, control, streaming confiable | Juegos, video en vivo, streaming rápido |

---

## ⚠️ Notas

* Si Windows muestra una alerta de firewall, permite el acceso en **red privada**.
* Todos los `.exe` deben ejecutarse en la **misma máquina** (localhost) para las pruebas iniciales.
* Usa `Ctrl+C` para detener el broker o los clientes.
* Si ves el mensaje `winsock_init failed`, revisa que no haya bloqueos de permisos o antivirus.

---

## ✨ Créditos

Desarrollado en C para el curso **Infraestructura de Comunicaciones** –
**Departamento de Ingeniería de Sistemas y Computación, Universidad de los Andes**
Profesor Asistente: *Nathalia Quiroga*

