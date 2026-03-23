idf_component_register(
    SRC_DIRS
        ../c_fonts
        ../c_elements
        ../c_pages
        ../c_graphics
        ../main
    INCLUDE_DIRS
        include
        ../boards
    REQUIRES
        fatfs
        sdmmc
        driver
    PRIV_REQUIRES
        app_update
        esp_http_server
        esp_wifi
        esp_netif
        esp_event
)

# ── ESP32-P4 (JC4880P433): link ST7701 LCD and PPA engine components ──
# These are in ../components/ and automatically registered by ESP-IDF.
# We just need to declare the dependency so the build system links them.
if(CONFIG_IDF_TARGET_ESP32P4)
    idf_component_get_property(main_lib ${COMPONENT_NAME} COMPONENT_LIB)
    target_link_libraries(${main_lib} PRIVATE st7701_lcd ppa_engine)
endif()
