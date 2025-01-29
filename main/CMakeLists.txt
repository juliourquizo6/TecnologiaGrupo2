idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES power_manager esp_netif)

# Incluye el archivo Kconfig para menuconfig
set(COMPONENT_ADD_INCLUDEDIRS .)	