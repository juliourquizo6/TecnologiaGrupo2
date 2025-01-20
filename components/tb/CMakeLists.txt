idf_component_register(SRCS "tb.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_event mqtt
                       PRIV_REQUIRES json esp_app_format esp_https_ota nvs_flash)
