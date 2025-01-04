# Componente WiFi (wifi_component.c)

## Descripción
Esta componente proporciona una solución para gestionar la conectividad Wi-Fi en dispositivos ESP32, esta se hace mediante aprovisionamiento mediante un punto de acceso (SoftAP).

## Características
- Provisión de credenciales Wi-Fi usando SoftAP.
- Uso del esquema de seguridad `WIFI_PROV_SECURITY_2` para un aprovisionamiento seguro.
- Integración con la pila de eventos de ESP-IDF.
- Compatibilidad con la inicialización y gestión de redes Wi-Fi.
- Soporte para gestión de bajo consumo con Wi-Fi.

## Dependencias
Esta componente depende de las siguientes bibliotecas y funciones de ESP-IDF:
- `esp_wifi`
- `esp_event`
- `wifi_provisioning/manager`
- `wifi_provisioning/scheme_softap`
- `nvs_flash`
- `esp_log`

## Configuración
No requiere configuración específica en `menuconfig`, pero el componente utiliza los parámetros de seguridad `sec2_salt` y `sec2_verifier` para garantizar una conexión segura durante el aprovisionamiento.

## Funciones de Bajo Consumo
Este componente incluye funciones para gestionar el consumo de energía del dispositivo ESP32:
- **`enable_wifi_low_power_mode()`**: Habilita el modo de bajo consumo para Wi-Fi.
- **`disable_wifi_low_power_mode()`**: Deshabilita el modo de bajo consumo para Wi-Fi.

## Uso
### Inicialización del Wi-Fi
Para iniciar y configurar el Wi-Fi con aprovisionamiento:
```c
#include "wifi_component.h"

void app_main() {
    wifi_init();
}
