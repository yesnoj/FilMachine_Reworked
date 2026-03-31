#
# Local "driver" component override.
#
# The built-in ESP-IDF "driver" component is a legacy compatibility wrapper.
# On ESP32-P4 its i2c/i2c.c has a global constructor (check_i2c_driver_conflict)
# that aborts when the new esp_driver_i2c API has already been used (e.g. by
# the GT911 managed-component touch driver).
#
# This override shadows the IDF built-in so we can compile a PATCHED version
# of i2c.c on P4 (with the constructor neutralised), while still forwarding
# all the public esp_driver_* dependencies so that any component that REQUIRES
# "driver" continues to resolve its deps.
#
# For non-P4 targets we pull in the real IDF driver sources unchanged.
#

# ── Bootloader build: register a no-op component ──
if(BOOTLOADER_BUILD)
    idf_component_register()
    return()
endif()

# ── Locate the real IDF driver component for reference ──
set(IDF_DRIVER_DIR "$ENV{IDF_PATH}/components/driver")

# ── Collect sources ──
set(srcs "driver_compat.c")                       # always present (placeholder)

# ── Always use the real IDF driver headers ──
# Our local empty directories don't contain the actual header files;
# the real component does.
set(includes
    "${IDF_DRIVER_DIR}/deprecated"
    "${IDF_DRIVER_DIR}/i2c/include"
    "${IDF_DRIVER_DIR}/touch_sensor/include"
    "${IDF_DRIVER_DIR}/twai/include"
)

if(IDF_TARGET STREQUAL "esp32p4")
    # ── ESP32-P4: compile a patched i2c.c ──
    # We need the legacy I2C functions (esp_lcd uses them), but must remove
    # the check_i2c_driver_conflict() global constructor that aborts when
    # esp_driver_i2c is also linked (GT911 touch driver).
    #
    # Strategy: read the real i2c.c, strip the __attribute__((constructor))
    # annotation so the conflict-check function still exists but never runs
    # automatically, then compile the patched copy.
    set(PATCHED_I2C "${CMAKE_CURRENT_BINARY_DIR}/i2c_patched.c")
    file(READ "${IDF_DRIVER_DIR}/i2c/i2c.c" _i2c_src)
    string(REPLACE "__attribute__((constructor))" "/* constructor disabled for P4 */"
           _i2c_src "${_i2c_src}")
    file(WRITE "${PATCHED_I2C}" "${_i2c_src}")
    list(APPEND srcs "${PATCHED_I2C}")

    message(STATUS "[driver override] ESP32-P4 — legacy i2c.c included with conflict constructor DISABLED")

else()
    # ── Non-P4: include the real IDF legacy driver sources ──
    # We mirror the conditionals from the real component's CMakeLists.txt.
    if(CONFIG_SOC_I2C_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/i2c/i2c.c")
    endif()
    if(CONFIG_SOC_GPTIMER_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/gptimer/gptimer.c"
                         "${IDF_DRIVER_DIR}/gptimer/gptimer_priv.c")
    endif()
    if(CONFIG_SOC_TWAI_SUPPORTED AND NOT CONFIG_SOC_TWAIFD_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/twai/twai.c")
    endif()
    if(CONFIG_SOC_ADC_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/adc_legacy.c")
    endif()
    if(CONFIG_SOC_DAC_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/dac_common_legacy.c"
                         "${IDF_DRIVER_DIR}/deprecated/${IDF_TARGET}/dac_legacy.c")
    endif()
    if(CONFIG_SOC_PCNT_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/pcnt_legacy.c")
    endif()
    if(CONFIG_SOC_RMT_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/rmt_legacy.c")
    endif()
    if(CONFIG_SOC_MCPWM_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/mcpwm_legacy.c")
    endif()
    if(CONFIG_SOC_SDM_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/sigma_delta_legacy.c")
    endif()
    if(CONFIG_SOC_I2S_SUPPORTED)
        list(APPEND srcs "${IDF_DRIVER_DIR}/deprecated/i2s_legacy.c")
    endif()
    if(CONFIG_SOC_TOUCH_SENSOR_SUPPORTED AND CONFIG_SOC_TOUCH_SENSOR_VERSION LESS 3)
        list(APPEND srcs "${IDF_DRIVER_DIR}/touch_sensor/touch_sensor_common.c")
        if(IDF_TARGET STREQUAL "esp32")
            list(APPEND srcs "${IDF_DRIVER_DIR}/touch_sensor/esp32/touch_sensor.c")
        elseif(IDF_TARGET STREQUAL "esp32s2")
            list(APPEND srcs "${IDF_DRIVER_DIR}/touch_sensor/esp32s2/touch_sensor.c")
        elseif(IDF_TARGET STREQUAL "esp32s3")
            list(APPEND srcs "${IDF_DRIVER_DIR}/touch_sensor/esp32s3/touch_sensor.c")
        endif()
    endif()

    message(STATUS "[driver override] Non-P4 — including real IDF legacy sources")
endif()

# ── Register the component ──
# Public REQUIRES mirrors what the real driver component exports so
# transitive dependencies (fatfs → sdmmc → driver → esp_driver_spi etc.) work.
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${includes}
    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_spi
        esp_driver_ledc
        esp_driver_uart
        esp_driver_parlio
        esp_driver_gptimer
        esp_driver_pcnt
        esp_driver_rmt
        esp_driver_mcpwm
        esp_driver_sdm
        esp_driver_twai
        esp_driver_i2s
        esp_driver_dac
        esp_driver_ana_cmpr
        esp_driver_touch_sens
        esp_driver_sdmmc
        esp_driver_sdspi
        esp_driver_cam
        esp_driver_jpeg
        esp_driver_isp
        esp_driver_ppa
        esp_driver_bitscrambler
        esp_driver_sdio
        esp_driver_tsens
        esp_driver_usb_serial_jtag
        esp_adc
        freertos
        esp_lcd
    PRIV_REQUIRES
        esp_timer
        esp_pm
        esp_ringbuf
        esp_mm
)
