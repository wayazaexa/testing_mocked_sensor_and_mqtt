| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- |

# Basic I2C Master Example

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Overview

This example started as Espressif's basic I2C master example. It has been
refactored for a course module about testing and quality assurance for embedded
systems:

- `sensor_bus` is the small interface between sensor logic and hardware.
- `mpu9250` contains testable sensor behavior.
- `fake_sensor_bus` is a controllable mock for unit tests.
- `app_net` optionally publishes the probe result over WiFi + MQTT.
- `tools/node_red` contains a local Mosquitto + Node-RED fake backend.

The regular firmware still demonstrates basic usage of the I2C driver by
reading and writing registers for an I2C-connected sensor.

If you have a new I2C application to go (for example, read the temperature data from external sensor with I2C interface), try this as a basic template, then add your own code.

## How to use example

### Hardware Required

To run this example, you should have an Espressif development board based on a chip listed in supported targets as well as a MPU9250. MPU9250 is a inertial measurement unit, which contains a accelerometer, gyroscope as well as a magnetometer, for more information about it, you can read the [datasheet of the MPU9250 sensor](https://invensense.tdk.com/wp-content/uploads/2015/02/PS-MPU-9250A-01-v1.1.pdf).

#### Pin Assignment

**Note:** The following pin assignments are used by default, you can change these in the `menuconfig` .

|                  | SDA             | SCL           |
| ---------------- | -------------- | -------------- |
| ESP I2C Master   | I2C_MASTER_SDA | I2C_MASTER_SCL |
| MPU9250 Sensor   | SDA            | SCL            |

For the actual default value of `I2C_MASTER_SDA` and `I2C_MASTER_SCL` see `Example Configuration` in `menuconfig`.

**Note:** There's no need to add an external pull-up resistors for SDA/SCL pin, because the driver will enable the internal pull-up resistors.

### Build and Flash

Enter `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

```bash
I (328) example: I2C initialized successfully
I (338) example: MPU9250 probe status=ok WHO_AM_I=0x71 bus_error=ESP_OK
I (338) example: MPU9250 reset command sent
I (338) example: I2C de-initialized successfully
```

## Run the Sensor Unit Tests

The test app under `test_apps/sensor_unit` uses Unity and the fake sensor bus.
It verifies the behavior without needing a real MPU9250. The tests run once automatically on boot.

```bash
cd test_apps/sensor_unit
. "$HOME/.espressif/tools/activate_idf_v6.0.sh"
idf.py set-target esp32c6
idf.py -p PORT build flash monitor
```

The same test app is also a good starting point for host/Linux target testing
when the local ESP-IDF environment supports it.

If ESP-IDF reports that the project was configured with `python3` but `python`
is currently active, or the opposite, clean once and keep using the same command
form afterwards:

```bash
idf.py fullclean
```

## Enable WiFi + MQTTS Telemetry

Telemetry is disabled by default so the I2C/sensor example stays small. Enable it with `menuconfig`:

```bash
. "$HOME/.espressif/tools/activate_idf_v6.0.sh"
idf.py menuconfig
```

Set these values under `Example Configuration -> WiFi and MQTT telemetry`:

- `Enable WiFi + MQTT telemetry`
- `WiFi SSID`
- `WiFi password`
- `MQTTS broker URI`, for example `mqtts://192.168.1.10:8883`
- `SNTP server for TLS certificate validation`, for example `pool.ntp.org`
- `Device ID`, for example `esp32c6-01`

In the text menu, use `/` to search for the symbol name if you cannot find it:

- `EXAMPLE_ENABLE_MQTT_TELEMETRY`
- `EXAMPLE_WIFI_SSID`
- `EXAMPLE_WIFI_PASSWORD`
- `EXAMPLE_MQTT_BROKER_URI`
- `EXAMPLE_SNTP_SERVER`
- `EXAMPLE_DEVICE_ID`

## Size-Oriented Build Settings

For the WiFi + MQTT firmware, use these `menuconfig` paths:

- `Partition Table -> Partition Table -> Single factory app, large`
- `Compiler options -> Optimization Level -> Optimize for size (-Os)`
- `Component config -> ESP-MQTT Configurations -> MQTT over SSL -> enabled`
- `Component config -> ESP-MQTT Configurations -> MQTT over WebSocket -> disabled`
- `Component config -> IEEE 802.15.4 -> Enable IEEE 802.15.4 -> disabled`
- `Component config -> Wi-Fi -> WiFi SoftAP Support -> disabled`
- `Component config -> Wi-Fi -> WiFi Enterprise Support -> disabled`
- `Component config -> Wi-Fi -> WPA3 SAE support -> disabled`

The project also includes `sdkconfig.defaults` with these defaults. If an old
`sdkconfig` already exists, either change it through `menuconfig` or regenerate
it from defaults:

```bash
idf.py fullclean
rm sdkconfig
idf.py build
```

The firmware publishes to:

```text
iot25/<device_id>/sensor/mpu9250
```

Example payload:

```json
{"device_id":"esp32c6-01","seq":1,"who_am_i":113,"status":"ok"}
```

## Run the Fake Backend

Mosquitto listens on `1883` for the internal Node-RED flow and on `8883` for
ESP32-C6 clients using MQTTS. The broker presents a server certificate on
`8883`; the ESP32-C6 validates that certificate with the embedded CA. This is
server-authenticated TLS, not mutual TLS, so Mosquitto does not request a client
certificate from the ESP32-C6.

### Generate local demo certificates before the first backend run from the project root:
In this example, 192.168.1.10 is the IP-adress of the computer where the MQTT-broker runs.

```bash
tools/node_red/generate_tls_certs.sh 192.168.1.10
idf.py build
```

The firmware embeds
`components/app_net/certs/ca.crt` and uses it to validate the broker certificate
served from `tools/node_red/certs/server.crt`.

Start Mosquitto and Node-RED:

```bash
cd tools/node_red
docker compose up
```

Open Node-RED at `http://localhost:1880`. The included flow subscribes to
`iot25/+/sensor/mpu9250`, shows incoming messages in the debug sidebar, and
publishes a fake backend response to `iot25/<device_id>/backend/status`.

MQTTS certificate validation also needs a correct device clock. After WiFi
connects, the firmware waits for SNTP before starting MQTT. If your classroom
network blocks NTP, use a reachable local NTP/SNTP server in `menuconfig` or the
TLS handshake will fail even when the CA and broker IP are correct.

## Troubleshooting

(For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you as soon as possible.)
