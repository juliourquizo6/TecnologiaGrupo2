#ifndef WIFI_COMPONENT_H
#define WIFI_COMPONENT_H

#include "esp_event.h"

void enable_wifi_low_power_mode(void); 
void disable_wifi_low_power_mode(void);

// Declaración de funciones de inicialización y eventos
void wifi_init(void);

#endif