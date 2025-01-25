idf_component_register(SRCS "tb_client.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_event json
                    PRIV_REQUIRES esp_app_format esp_https_ota esp_wifi mbedtls mqtt nvs_flash)
