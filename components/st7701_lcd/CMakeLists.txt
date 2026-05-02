if(IDF_TARGET STREQUAL "esp32p4")
    idf_component_register(
        SRCS "st7701_lcd.c" "esp_lcd_st7701.c" "esp_lcd_st7701_mipi.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_lcd esp_hw_support esp_driver_gpio esp_driver_ledc ppa_engine
        PRIV_REQUIRES esp_mm esp_driver_ppa esp_timer
    )
else()
    idf_component_register(
        INCLUDE_DIRS "include"
    )
endif()
