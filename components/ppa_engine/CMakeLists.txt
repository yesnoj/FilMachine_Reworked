if(CONFIG_IDF_TARGET_ESP32P4)
    idf_component_register(
        SRCS "ppa_engine.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_driver_ppa esp_psram
    )
else()
    idf_component_register(
        INCLUDE_DIRS "include"
    )
endif()
