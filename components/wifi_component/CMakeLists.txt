idf_component_register(SRCS "wifi_component.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_event json nvs_flash esp_wifi wifi_provisioning)
