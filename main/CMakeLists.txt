idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_event esp_netif nvs_flash protocol_examples_common sgp30 tb_client)
