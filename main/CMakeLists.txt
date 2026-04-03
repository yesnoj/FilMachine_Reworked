# ── Conditional dependencies ──
# ESP32-P4 gets WiFi via ESP-Hosted (C6 companion chip over SDIO)
# esp_wifi_remote provides the standard esp_wifi API transparently
# ESP32-P4 needs st7701_lcd + ppa_engine for MIPI-DSI display
if(IDF_TARGET STREQUAL "esp32p4")
    set(WIFI_DEPS esp_wifi esp_netif)
    set(P4_DEPS st7701_lcd ppa_engine)
    # ESP32-P4: use individual esp_driver_* components instead of the legacy
    # "driver" compat wrapper, which would pull in the old i2c.c and conflict
    # with the new esp_driver_i2c used by GT911 touch.
    set(DRIVER_DEPS esp_driver_gpio esp_driver_ledc esp_driver_i2c esp_driver_spi)
else()
    set(WIFI_DEPS esp_wifi esp_netif)
    set(P4_DEPS "")
    set(DRIVER_DEPS driver)
endif()

idf_component_register(
    SRC_DIRS
        ../c_fonts
        ../c_elements
        ../c_pages
        ../drivers
        ../main
    INCLUDE_DIRS
        include
        ../drivers/include
        ../boards
    REQUIRES
        fatfs
        sdmmc
        ${DRIVER_DEPS}
    PRIV_REQUIRES
        app_update
        esp_http_server
        esp_event
        mdns
        ${WIFI_DEPS}
        ${P4_DEPS}
)

# ── Board selection ──
if(IDF_TARGET STREQUAL "esp32p4")
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BOARD_JC4880P433)
else()
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BOARD_MAKERFABS_S3)
endif()
