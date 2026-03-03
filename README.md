# nrf9151-sensor-gateway

A modular LTE-M to CoAP sensor gateway running on the nRF9151 DK with Zephyr RTOS.
The firmware collects readings from multiple data sources (I2C, SPI, GPIOs, …),
serializes them as JSON snapshots, and forwards them over LTE-M via CoAP to a lightweight
C server that parses each snapshot and stores every reading in a SQLite database.

The focus of this repo is the **data-source abstraction layer**: each physical sensor (or
any data-producing peripheral) implements a two-function interface (`init` / `read`), and
adding a new one requires no changes to the core pipeline.

> **Note:** Builds on the CoAP transport layer explored in
> [nrf9151-coap-backends](https://github.com/savosaicic/nrf9151-coap-backends).
> The `coap_backend_t` interface is carried over unchanged; only the libcoap3 backend
> is wired up here.

## Prerequisites

### Hardware
- A nRF9151 board with a SIM card

### Software — firmware
- nRF Connect SDK + Zephyr SDK toolchain
(follow the [Getting Started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html))
- `libcoap-3`, `cJSON`, `SQLite3` (for the server — see Server Setup below)

## Firmware Setup

### 1. Initialise the workspace

```bash
mkdir nrf9151-sensor-gateway-ws && cd nrf9151-sensor-gateway-ws
west init -m https://github.com/savosaicic/nrf9151-sensor-gateway --mr main
west update
pip install -r zephyr/scripts/requirements.txt
```

### 2. Set the server address

Open `firmware/app/prj.conf` and set your server's hostname or IP:

```
CONFIG_COAP_SERVER_HOSTNAME="your-server.example.com"
```

All options (hostname, port, resource path, sampling interval) can also be set interactively via menuconfig:

```bash
west build -b nrf9151dk/nrf9151/ns firmware/app -t menuconfig
```

### Build & flash

```bash
west build -b nrf9151dk/nrf9151/ns firmware/app
west flash
```

## Server Setup

Make sure UDP port `5683` is reachable on your server.

### Native build

```bash
# Install dependencies
sudo apt install libcoap3-dev libcjson-dev libsqlite3-dev

# Build
cd server/
make

# Run  (pass a database file path as the only argument)
./coap_sensor_server sensors.db
```

### Docker

```bash
cd server/
docker build -t coap-sensor-server .
docker run --rm -p 5683:5683/udp -v $(pwd)/data:/data coap-sensor-server /data/sensors.db
```

Incoming snapshots are printed to stdout and written to the database as they arrive.
