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

## Testing

The application should be tested under the following conditions:

1. **MQTT Connection:** Start the application while the Mosquitto broker is running and verify that it connects successfully.
2. **Multiple Devices:** Run the Python simulator and verify that telemetry from multiple devices is received and processed.
3. **High Temperature:** Verify that a `HIGH_TEMPERATURE` alert is generated when temperature exceeds 30 °C.
4. **High Humidity:** Verify that a `HIGH_HUMIDITY` alert is generated when humidity exceeds 80%.
5. **Low Battery:** Verify that a `LOW_BATTERY` alert is generated when battery falls below 20%.
6. **Malformed JSON:** Send an invalid JSON message and verify that the application handles it without crashing.
7. **Reconnection:** Stop the Mosquitto broker while the application is running, restart it, and verify that the application reconnects.
8. **Graceful Shutdown:** Press `Ctrl+C` and verify that the application shuts down cleanly.

## Test Results

| Test | Result |
|---|---|
| MQTT connection | Successful |
| Multiple devices | Successful |
| High temperature alert | Successful |
| High humidity alert | Successful |
| Low battery alert | Successful |
| Malformed JSON handling | Successful |
| Broker reconnection | Successful |
| Graceful shutdown | Successful |

## Known Limitations

- **Fixed alert thresholds:** Temperature, humidity, and battery thresholds are predefined in the source code.
- **Limited device capacity:** The application can track a maximum of 100 devices.
- **In-memory device storage:** Device information is stored in memory and is lost when the application shuts down.
- **No offline device detection:** The application does not currently detect when an individual IoT device stops sending telemetry.
- **No persistent alert history:** Alerts are published to MQTT, but the application does not maintain a separate permanent alert history.
- **Basic logging:** Output can be written to a file, but currently only logs alerts.
