<<<<<<< HEAD
idf_component_register(SRCS "app_main.c" "power_manager.c"
                    INCLUDE_DIRS ".")
=======
idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES power_manager esp_netif)

# Incluye el archivo Kconfig para menuconfig
set(COMPONENT_ADD_INCLUDEDIRS .)	
>>>>>>> 02a22f5f3bc276618fede28967c17caa49119fe5
