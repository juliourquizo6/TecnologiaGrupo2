# tb_client

## Description

The `tb_client` component provides an interface to connect and interact with ThingsBoard using MQTT and HTTPS. It handles the initialization, provisioning, OTA updates, attribute callbacks and telemetry data transmission to ThingsBoard.

The internal state of the client restarts when something goes wrong so that it may be fault tolerant.

## Features

- Connects to ThingsBoard using MQTT.
- Handles device provisioning with ThingsBoard.
- Sends telemetry data to ThingsBoard.
- Receives and processes attribute updates from ThingsBoard.
- Supports firmware updates via HTTPS OTA.

## Configuration

The component can be configured using the following options in the `Kconfig` file:

- `TB_DEVICE_NAME`: Device name in ThingsBoard.
- `TB_PROVISION_DEVICE_KEY`: Provisioning device key, obtained from the configured device profile.
- `TB_PROVISION_DEVICE_SECRET`: Provisioning device secret, obtained from the configured device profile.

## API

### Initialization

```c
esp_err_t tb_client_init(const char *hostname, const char *telemetry_topic, void (*attributes_callback)(cJSON *));
```

Initializes the ThingsBoard client.

- `hostname`: The hostname of the ThingsBoard server. If `NULL`, defaults to `demo.thingsboard.io`.
- `telemetry_topic`: The MQTT topic for telemetry data. If `NULL`, defaults to `v1/devices/me/telemetry`.
- `attributes_callback`: Callback function to handle attribute updates from ThingsBoard.

### Sending Telemetry Data

```c
void tb_client_send_telemetry(const cJSON *telemetry);
```

Sends telemetry data to ThingsBoard.

- `telemetry`: A `cJSON` object containing the telemetry data.

## Example Usage

```c
#include "cJSON.h"
#include "tb_client.h"

void attributes_callback(cJSON *attributes) {
    // Handle attribute updates
}

void app_main(void) {
    // Initialize the ThingsBoard client with default values
    tb_client_init(NULL, NULL, attributes_callback);

    // Create telemetry data
    cJSON *telemetry = cJSON_CreateObject();
    cJSON_AddNumberToObject(telemetry, "temperature", 25.5);

    // Send telemetry data
    tb_client_send_telemetry(telemetry);

    // Free the telemetry data
    cJSON_Delete(telemetry);
}
```
