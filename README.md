# tecnologia

ESP32-based IoT project for monitoring environmental parameters using CO2 sensors and ThingsBoard integration.

## Features

- CO2 and TVOC monitoring using SGP30 sensor
- Wi-Fi provisioning with secure authentication
- MQTT integration with ThingsBoard
- Over-the-air (OTA) firmware updates
- Dynamic sampling rate configuration
- Secure device provisioning

## Project Structure

```
tecnologia/
├── components/
│   ├── power_manager/      # Deep sleep schedule management
│   ├── sgp30/              # CO2 sensor driver and handler
│   ├── tb_client/          # ThingsBoard MQTT client
│   └── wifi_component/     # Wi-Fi provisioning and connection
├── main/                   # Main application code
└── README.md
```

## Hardware Requirements

- ESP32 development board
- SGP30 CO2/TVOC sensor
- I2C connection:
  - SDA: Configured GPIO pin
  - SCL: Configured GPIO pin

## Features

### Sensor Capabilities
- CO2 measurement (eCO2)
- Total Volatile Organic Compounds (TVOC)
- Configurable sampling and reporting rates
- Automatic baseline calibration

### Connectivity
- Secure Wi-Fi provisioning
- MQTT communication with ThingsBoard
- Automatic reconnection handling
- Device provisioning support

### Remote Management
- OTA firmware updates
- Dynamic configuration via ThingsBoard
- Real-time telemetry reporting
- Attribute synchronization

## Building and Flashing

1. Set up ESP-IDF environment
2. Configure project:
   ```bash
   idf.py menuconfig
   ```
3. Build:
   ```bash
   idf.py build
   ```
4. Flash:
   ```bash
   idf.py flash monitor
   ```

## Configuration

Key parameters can be configured in menuconfig:
- Wi-Fi provisioning credentials
- Sampling rates
- ThingsBoard connection details
- Device name and credentials
