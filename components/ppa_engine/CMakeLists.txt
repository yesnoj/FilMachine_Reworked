if(IDF_TARGET STREQUAL "esp32p4")
    idf_component_register(
        SRCS "ppa_engine.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_psram
        PRIV_REQUIRES esp_driver_ppa esp_mm
    )
else()
    idf_component_register(
        INCLUDE_DIRS "include"
    )
endif()
