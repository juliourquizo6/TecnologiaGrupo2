idf_component_register(SRCS "tb.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_event json
                       PRIV_REQUIRES esp_app_format esp_https_ota mqtt nvs_flash)
