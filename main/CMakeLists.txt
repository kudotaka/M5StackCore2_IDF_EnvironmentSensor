set(SOURCES main.c)
set(COMPONENT_REQUIRES m5core2 m5unit lvgl lvgl_esp32_drivers)
idf_component_register(SRCS main.c wifi.c ui.c INCLUDE_DIRS "includes")