idf_component_register(SRCS "app_main.c" "power_manager.c"
                                INCLUDE_DIRS "."
                                REQUIRES esp_timer esp_sntp nvs_flash)