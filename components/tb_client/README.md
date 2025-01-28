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
- Manages wifi power modes

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

## Requirements

It implements all the project requirements listed below:

- Actualización remota del software (OTA).

El sistema permitirá su actualización mediante OTA seguro (HTTPS) siguiendo el modelo OTA de ThingsBoard. Se deberán poder iniciar campañas de OTA desde ThingsBoard

El firmware debe estar correctamente versionado (puedes seguir las pautas sobre numeración de versiones propias de Espressif)

Firmado de software recibido mediante OTA. ¡Cuidado con no modificar la placa para forzar siempre el arranque seguro! Sólo se consultará si están firmados los binarios recibidos por OTA no el de fábrica (por ejemplo, usando SHA).

- Tolerancia a fallos.

El software se debe diseñar de forma que reaccione a posibles contratiempos

Debe contemplarse la posibilidad de que no se consiga conexión WiFi o se pierda en determinados instantes.

- Reducción de consumo:

El nodo deberá usar convenientemente los modos de bajo consumo WiFi.

- Definición y creación de objetos:

Obligatoriamente se utilizará MQTT como protocolo a nivel de aplicación para la definición/jerarquización de objetos, y transferencia de datos.

Se diseñará una jerarquía de topics que permita identificar a cada sensor en su Facultad, Piso, Aula y número y tipo de sensor, al menos.

Se añadirán las capas de seguridad necesarias sobre MQTT (SSL/TLS y usuario/contraseña).

- Seguridad y representación de datos:

Se utilizará JSON o CBOR para el intercambio de payloads. Los alumnos decidirán y justificarán la razón para usar cada uno de dichos formatos en cada punto del despliegue.

Se valorará el/los tipo(s) de datos seleccionados en función de los rangos de valores y precisión deseada para cada sensor, así como su codificación o compresión para reducir el tráfico generado.

En todo caso, la transferencia de datos deberá aplicar algún tipo de mecanismo de cifrado de extremo a extremo.

- Dashboard:

Se desarrollará un dashboard que permita la visualización, en tiempo real, de los valores sensorizados para todos los sensores desplegados.

La representación de datos, así como la gestión de los nodos, se llevará a cabo utilizando la plataforma Thingsboard a modo de dashboard.

El dashboard permitirá, además, la comunicación bidireccional con cada sensor (e.g. para configurar alguno de sus parámetros).

- Configuración y gestión del despliegue:

En la medida de lo posible, el firmware deberá ser configurable en el proceso de compilación vía menuconfig en aquellos parámetros que sean estáticos y puedan definirse en tiempo de compilación.

En aquellos casos en los que un valor de configuración pueda ser reconfigurado dinámicamente, dicha comunicación se realizará utilizando MQTT o un protocolo similar, y deseablemente se dará soporte para su modificación vía panel de control, una CLI específica o el sistema de atributos de Thingsboard.
Se monitorizarán y notificarán (por ejemplo, usando LWT) las caídas de nodos, así como otros eventos de interés.

Se dará soporte a comunicación bidireccional para controlar remotamente, por ejemplo, la frecuencia de muestreo de los sensores, usando por ejemplo el sistema de atributos de Thingsboard.

Se implementarán, en colaboración con ANIOT, sistemas de actualización remota (OTA) del firmware, dotando al sistema de medidas de seguridad en el mismo. Este sistema podrá estar basado en Thingsboard.

- Diseño e implementación de una estrategia de provisioning

El firmware original debe ser idéntico para todos los nodos, pero en la etapa de provisioning debe conseguir  información específica: identificación del nodo (de forma obligatoria)~~, e identificación del espacio, (aula/laboratorio/despacho, número, ubicación dentro de dicho espacio)~~.
