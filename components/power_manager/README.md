# Gestor de Energía para Monitorización de Calidad del Aire

Este componente está diseñado para gestionar la energía de un dispositivo basado en ESP32, optimizando el consumo en función de un horario operativo predefinido. El gestor de energía permite que el dispositivo entre en modo de **Deep Sleep** fuera del horario operativo para ahorrar batería.

## Características

- **Gestión del Horario Operativo**: Configura un horario operativo para que el dispositivo funcione solo dentro de ese intervalo de tiempo. Fuera de ese intervalo, el dispositivo entra en modo de **Deep Sleep**.
- **Sincronización Horaria (NTP)**: Sincronización automática del reloj del dispositivo con servidores NTP.
- **Temporizadores de Sueño**: Soporta la configuración de temporizadores de sueño para optimizar el consumo de energía.
- **Configuración Flexible**: Los horarios y los tiempos de sueño pueden configurarse fácilmente.
- **Optimización del Consumo**: Permite reducir el consumo de energía en horas no operativas, lo cual es fundamental para dispositivos IoT que dependen de baterías.

## Requisitos

- **ESP32**: Este componente está diseñado para funcionar en dispositivos ESP32 con el entorno de desarrollo ESP-IDF.
- **ESP-IDF**: Este componente requiere el uso de ESP-IDF (versión 4.x o superior).
- **Conexión a Internet**: El componente utiliza NTP para sincronizar el tiempo, por lo que se necesita acceso a una red para obtener la hora correcta.
- **Configuración de Hardware**: Asegúrate de que tu hardware sea compatible con los modos de sueño de ESP32 (Deep Sleep).

## Instalación y Configuración

1. **Instalación de ESP-IDF**:
   - Si no has instalado ESP-IDF, sigue las instrucciones de instalación en [la documentación oficial de ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/).
   
2. **Configuración de Horario Operativo**:
   - Modifica los valores de las siguientes macros en `power_manager.h` para definir el horario operativo:
     ```c
     #define OPERATIVE_HOUR_START 8   // Hora de inicio (8:00)
     #define OPERATIVE_HOUR_END 22    // Hora de fin (22:00)
     ```

3. **Configuración de Zona Horaria**:
   - Asegúrate de configurar la zona horaria adecuada en `power_manager.c`. En el ejemplo actual, está configurado como `"UTC+1"`:
     ```c
     #define TIME_ZONE "UTC+1"
     ```

4. **Configuración de Temporizadores de Sueño**:
   - El tiempo de sueño puede configurarse en las macros:
     ```c
     #define SLEEP_TIMER 10 * 60 * 60 * 1000000ULL  // 10 horas
     ```
   - Ajusta los valores según tus necesidades.

5. **Sincronización Horaria**:
   - Este componente usa **SNTP** para la sincronización horaria. Asegúrate de habilitar el servicio de NTP en el menú de configuración de ESP-IDF (`idf.py menuconfig`):
     - Navega a **Component config** -> **LWIP** -> **SNTP** y habilita la opción.

6. **Configuración en `Kconfig.projbuild`**:
   - Si necesitas opciones adicionales en `menuconfig`, asegúrate de tener configurado `Kconfig.projbuild` con las opciones necesarias.

## Funcionalidad

### Inicialización del Gestor de Energía
Al iniciar el dispositivo, el componente **Gestor de Energía** realiza lo siguiente:
1. **Configuración de zona horaria** y sincronización horaria mediante NTP.
2. **Inicialización del temporizador** para gestionar las transiciones entre el horario operativo y el modo de sueño.
3. **Configuración de temporizadores de sueño** basados en los valores predeterminados o configurados por el usuario.

### Temporizadores
El gestor de energía tiene dos tipos principales de temporizadores:
- **Temporizador de prueba** (`TIMER_PRUEBA`): Se usa para probar el comportamiento inicial del sistema.
- **Temporizador de sueño** (`SLEEP_TIMER`): Define cuánto tiempo el dispositivo permanecerá en **Deep Sleep** cuando no está en el horario operativo.

### Modos de Operación
El gestor de energía realiza lo siguiente dependiendo de la hora actual:
- **Dentro del horario operativo**: El dispositivo sigue funcionando y esperando el fin del horario operativo para entrar en sueño.
- **Fuera del horario operativo**: El dispositivo entra en **Deep Sleep** para ahorrar energía.

### Sincronización NTP
- La sincronización horaria se realiza mediante el protocolo **SNTP**. El dispositivo obtiene la hora de un servidor NTP para configurar correctamente los temporizadores de sueño y operar dentro del horario establecido.

### Función `timer_cb`
Esta función se ejecuta cuando el temporizador se activa. Si el dispositivo está dentro del horario operativo, calcula el tiempo restante hasta el fin del día operativo y configura el siguiente temporizador. Si está fuera del horario operativo, el dispositivo entra en **Deep Sleep**.

### Función `sync_hour`
Esta función sincroniza la hora del sistema con el servidor NTP y ajusta los temporizadores para que el dispositivo opere correctamente en el siguiente ciclo.

## Archivos del Proyecto

- **`power_manager.c`**: Contiene la implementación del gestor de energía, incluyendo la gestión de horarios, sincronización horaria, y modos de sueño.
- **`power_manager.h`**: Proporciona las declaraciones de las funciones y las configuraciones de las macros.
- **`app_main.c`**: Contiene la función principal del proyecto, donde se inicializa el gestor de energía.
- **`Kconfig.projbuild`**: Configuración específica del proyecto para habilitar características de ESP-IDF.
- **`CMakeLists.txt`**: Configuración del sistema de construcción para el proyecto, que incluye la configuración de componentes y dependencias.

## Ejemplo de Uso

```c
#include "power_manager.h"

void app_main(void) {
    // Inicialización del gestor de energía
    power_manager_init();

    ESP_LOGI(TAG, "Gestor de energía inicializado.");
}

