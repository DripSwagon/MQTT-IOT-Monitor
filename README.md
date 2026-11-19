# MQTT IoT Device Monitor

## Description

A C application that monitors IoT devices using MQTT.

## Features

- Connects to an MQTT broker using libmosquitto.
- Subscribes to `iot/devices/+/telemetry`.
- Parses incoming JSON messages using cJSON.
- Tracks temperature, humidity, and battery levels for multiple devices.
- Detects high temperature, high humidity, and low battery conditions.
- Publishes alerts to individual device topics.
- Supports command-line arguments.
- Handles connection interruptions and reconnections.
- Supports graceful shutdown using Ctrl+C.

## Requirements

- Linux
- GCC
- Mosquitto MQTT broker
- libmosquitto
- cJSON
- Python 3 (for the simulator)

## Installation

Install the required packages:

```bash
sudo apt update
sudo apt install build-essential mosquitto mosquitto-clients libmosquitto-dev libcjson-dev python3 python3-venv -y
```

## Build

Compile the application:

```bash
make
```

Remove compiled files:

```bash
make clean
```

## Running

Start the Mosquitto broker:

```bash
sudo systemctl start mosquitto
```

Run the application:

```bash
./iot_monitor
```

Optional arguments:

- `-H HOST`: MQTT broker hostname.
- `-P PORT`: MQTT broker port.
- `-o FILE`: Save output to a file.
- `-h`: Display help.

Example:

```bash
./iot_monitor -H localhost -P 1883 -o monitor.log
```

## Python Simulator

Create and activate a virtual environment:

```bash
python3 -m venv venv
source venv/bin/activate
```

Install the MQTT library:

```bash
pip install "paho-mqtt>=2.0"
```

Start the simulator in another terminal:

```bash
python mqtt_simulator.py
```

## Alert Thresholds

- `Temperature`:  Above 30 °C -> HIGH_TEMPERATURE
- `Humidity`:  Above 80% -> HIGH_HUMIDITY
- `Battery`:  Below 20% -> LOW_BATTERY

## Shutdown

Press `Ctrl+C` to stop the application gracefully.
