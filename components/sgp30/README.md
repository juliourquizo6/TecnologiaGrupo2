# SGP30

This component provides an interface for the SGP30 air quality sensor, allowing for the measurement of TVOC (Total Volatile Organic Compounds) and eCO2 (equivalent CO2) levels.

## Features

- Initialize and configure the SGP30 sensor.
- Measure air quality parameters (TVOC and eCO2).
- Retrieve and set baseline values for calibration.
- Set absolute humidity for compensation.

## Requirements

- ESP-IDF framework
- FreeRTOS
- cJSON library

## Configuration

The component can be configured using the `Kconfig.projbuild` file. The following parameters can be set:

- `T_LECTURA`: Frequency of sensor readings in milliseconds.
- `T_ENVIO`: Frequency of data transmission in milliseconds.

## Usage

### Initialization

To initialize the SGP30 sensor, call the `sensor_co2_create` function with a handler for processing the sensor data.

```c
#include "sensor_co2.h"

void data_handler(const cJSON *data) {
    // Process sensor data
}

void app_main() {
    sensor_co2_create(data_handler);
}
```

### Setting Frequencies

You can set the reading and transmission frequencies using the following functions:

```c
void sensor_co2_set_frecuencia_lectura(int frec_ms);
void sensor_co2_set_frecuencia_envio(int frec_ms);
```

### Example

```c
#include "sensor_co2.h"

void data_handler(const cJSON *data) {
    // Process sensor data
    printf("TVOC: %d, eCO2: %d\n", cJSON_GetObjectItem(data, "tvoc")->valueint, cJSON_GetObjectItem(data, "co2")->valueint);
}

void app_main() {
    sensor_co2_create(data_handler);
    sensor_co2_set_frecuencia_lectura(2000); // Set reading frequency to 2000 ms
    sensor_co2_set_frecuencia_envio(5000);   // Set transmission frequency to 5000 ms
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
