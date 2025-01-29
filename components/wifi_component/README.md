# Componente WiFi (wifi_component.c)

## Descripción
Esta componente proporciona una solución para gestionar la conectividad Wi-Fi en dispositivos ESP32, esta se hace mediante aprovisionamiento mediante un punto de acceso (SoftAP). Además, está preparado para recibir también la url de Thingsboard para su almacenamiento en el NVS y su uso posterior.

## Características
- Provisión de credenciales Wi-Fi y url de Thingsboard usando SoftAP.
- Uso del esquema de seguridad `WIFI_PROV_SECURITY_2` para un aprovisionamiento seguro.
- Integración con la pila de eventos de ESP-IDF.
- Compatibilidad con la inicialización y gestión de redes Wi-Fi.
- Almacenamiento de los datos recibidos por aprovisionamiento en el NVS.

## Dependencias
Esta componente depende de las siguientes bibliotecas y funciones de ESP-IDF:
- `esp_wifi`
- `esp_event`
- `esp_netif`
- `wifi_provisioning/manager`
- `wifi_provisioning/scheme_softap`
- `nvs`
- `nvs_flash`
- `esp_log`

## Configuración
Ciertos parámetros son configurables por `menuconfig`; el usuario y la contraseña del dispositivo durante el aprovisionamiento (aunque no está implementada la generación del salt y del verifier en base a ellos si no se usan los default) y también el tiempo de espera entre dada reintento de reconexión a la wifi. Además el componente utiliza los parámetros de seguridad `sec2_salt` y `sec2_verifier` para garantizar una conexión segura durante el aprovisionamiento.

## Uso
### Inicialización del Wi-Fi
Para iniciar y configurar el Wi-Fi con aprovisionamiento:
```c
#include "wifi_component.h"

void app_main() {
    provision_and_connect();
}
```
### Aprovisionamiento
El dispositivo se convierte en un AP y genera su propia red wifi. Entonces, deberá conectarse a dicha red el dispositivo que desee provisionar las credenciales wifi y la url de Thingsboard y ejecutar la aplicación python de aprovisionamiento pasando como parámetros las credenciales wifi y la url de thingsboard.
Un ejemplo de ejecución de la aplición sería:
```bash
python provision.py --transport softap --service_name "192.168.4.1:80" --sec_ver 2 --ssid MIOT --passphrase MIOT_WIFI_2024! --custom_data demo.thingsboard.io
```
### Almacenamiento url thingsboard
La url de thingsboard que se recibe en el aprovisionamiento se almacena en la variable thingsboard_url (tanto al recibirla como si el dispositivo ya se encuentra provisionado), la cual es external y por tanto accesible desde el main.
