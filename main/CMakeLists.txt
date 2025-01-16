idf_component_register(SRCS "app_main.c"
                            "sgp30_sensor.c"
                            "power_manager.c"
                            "scheduler.c"
                    INCLUDE_DIRS ".")
# Incluye el archivo Kconfig para menuconfig
set(COMPONENT_ADD_INCLUDEDIRS .)