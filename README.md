# Air Quality Monitor

Este proyecto implementa un sistema de monitorización de calidad del aire usando ESP-IDF, con un sensor SGP30.

## Funcionalidades
- Lectura periódica de CO2 y TVOC.
- Configuración de frecuencias mediante `menuconfig`.
- Gestión de energía con modos de bajo consumo y deep sleep.
- Horarios configurables para funcionamiento.

## Estructura
- `sgp30_sensor.*`: Módulo del sensor SGP30.
- `power_manager.*`: Gestión de energía.
- `scheduler.*`: Gestión de horarios.

## Requisitos
- ESP-IDF v4.4 o superior.
- Sensor SGP30 conectado a I2C.

## Uso
1. Configura los parámetros en `menuconfig`.
2. Compila y flashea el firmware.
3. Monitorea las lecturas a través de UART.
