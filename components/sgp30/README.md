# SGP30

El componente "sgp30" proporciona una interfaz para el sensor de calidad del aire SGP30, que permite medir los niveles de TVOC (compuestos orgánicos volátiles totales) y eCO2 (CO2 equivalente).

## Características

- Inicialización y configuración del sensor SGP30.
- Obtención y manipulción de los datos generados del sensor SGP30, dependiendo de una frecuencia parametizable.
- Conversión de datos a un objeto JSON, para su posterior envío.

## Requisitos

- Framework: ESP-IDF
- Librerías: cJSON, AS7262, SGP30
- FreeRTOS

## Configuración

El componente puede configurarse mediante el archivo `Kconfig.projbuild`. Se pueden configurar los siguientes parámetros:

- `T_LECTURA`: Frecuencia de las lecturas del sensor SGP30 en milisegundos.
- `T_ENVIO`: Frecuencia de transmisión de datos en milisegundos.

## Explicación de las funcionalidades implementadas

### I2C

Se incluye código de la API oficial de Espressif para la configuración del I2C.
Tanto `i2c_init` como `main_i2c_read` y `main_i2c_write` son funcionalidades de la API oficial y están adaptadas a este proyecto.

 - `i2c_init`: Inicialización del bus I2C en modo maestro: Se asignan los pines GPIO para SDA y SCL, habilitando sus resistencias pull-up, y se configura la frecuencia del reloj. Finalmente, se instala el controlador I2C en el ESP32 para habilitar la comunicación con dispositivos esclavos.

### SGP30

 - `sgp30_co2_init`: Se inicializa el bus i2c para la correcta inicializacion del sensor. Posteriormente, se inicializan dos timers: una para la lectura del sensor y otro para el envío de datos. Por último, se crea y se inicializa un timer de 15 segundos el cual activa los dos timers anteriores. Estos 15 segundos son necesarios para que el sensor pueda calibrarse.

### Timers

 - `set_timers`: Se establecen ambos temporizadores con la frecuencia previamente definida en la configuración del proyecto (`Kconfig.projbuild`).
 - `sensor_co2_set_frecuencia_lectura` y `sensor_co2_set_frecuencia_envio`: El proposito de estas funciones es ser llamadas cuando se quiera modificar las frecuencias de lectura y envio y volver a arrancar los temporizadores con estas nuevas frecuencias.
 - `tm_lectura`: Se implementa una ecuación que calcula la media incremental de los valores eCO2 y TV0C y en cada llamada al callback se guardan los datos calculados en una estructura de datos.
 - `tm_envio`: Se guarda todos los datos recopilados en un JSON y se envía mediante el handler del sensor. Al final, se borra la información de la estructura de datos y se resetea el contador.


## Uso

```c
#include "sensor_co2.h"

void data_handler(const cJSON *data) {
    printf("TVOC: %d, eCO2: %d\n", cJSON_GetObjectItem(data, "tvoc")->valueint, cJSON_GetObjectItem(data, "co2")->valueint);
}

void app_main() {
    sensor_co2_create(data_handler);
    sensor_co2_set_frecuencia_lectura(2000);
    sensor_co2_set_frecuencia_envio(5000);
}
```
