#ifndef WIFI_COMPONENT_H
#define WIFI_COMPONENT_H

#include "esp_event.h"

// Declaración de funciones de inicialización y eventos
void wifi_init(void);
void wifi_register_event_handlers();

#endif