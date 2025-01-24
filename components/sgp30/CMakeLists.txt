idf_component_register(SRCS "sensor_co2.c" "src/sgp30.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer json driver)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format -Wno-Wunused-variable)
