idf_component_register(SRCS "wifi_component.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_event json esp_wifi nvs_flash wifi_provisioning)
