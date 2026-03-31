cmake_minimum_required(VERSION 3.14)

# ══════════════════════════════════════════════════════════════════
# Dual-purpose CMakeLists.txt
#   • idf.py build  → ESP-IDF firmware (sets ESP_PLATFORM=1)
#   • cmake .. && make → PC simulator (SDL2 + LVGL)
# ══════════════════════════════════════════════════════════════════

if(ESP_PLATFORM)
    # ─────────────────────────────────────────────
    # ESP-IDF firmware build
    # ─────────────────────────────────────────────

    # ESP32-P4 I2C conflict: the built-in "driver" compat component has a
    # global constructor in i2c/i2c.c that aborts when the new esp_driver_i2c
    # is also linked (used by GT911 touch).  We override "driver" with a local
    # component in components/driver/ that forwards the public REQUIRES but
    # does NOT compile i2c.c.  No EXCLUDE_COMPONENTS needed — the local
    # component shadows the IDF built-in automatically.

    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(FilMachine)

else()
    # ─────────────────────────────────────────────
    # PC Simulator build
    # ─────────────────────────────────────────────
    project(FilMachine_Simulator C)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    link_directories(${SDL2_LIBRARY_DIRS} /opt/homebrew/lib)

    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/lvgl/CMakeLists.txt")
        message(STATUS "Cloning LVGL v9.2.2 ...")
        execute_process(
            COMMAND git clone --depth 1 --branch v9.2.2 https://github.com/lvgl/lvgl.git
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()

    set(LV_CONF_PATH "${CMAKE_SOURCE_DIR}/lvgl_config/lv_conf.h" CACHE STRING "" FORCE)
    add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
    include_directories(${CMAKE_SOURCE_DIR}/lvgl_config)
    include_directories(${SDL2_INCLUDE_DIRS})
    add_subdirectory(lvgl)

    set(SIMULATOR_SOURCES
        src/main.c
        stub/esp_stubs.c
        stub/fatfs_stubs.c
        stub/freertos_stubs.c
    )

    set(FILMACHINE_SOURCES
        main/accessories.c
        main/ota_update.c
        main/ws_server.c
        main/ui_profile.c
        drivers/mcp23017.c
        drivers/ds18b20.c
        drivers/pca9685.c
        drivers/sensors.c
        c_pages/page_menu.c
        c_pages/page_processes.c
        c_pages/page_processDetail.c
        c_pages/page_stepDetail.c
        c_pages/page_settings.c
        c_pages/page_tools.c
        c_pages/page_checkup.c
        c_elements/element_process.c
        c_elements/element_step.c
        c_elements/element_filterPopup.c
        c_elements/element_messagePopup.c
        c_elements/element_rollerPopup.c
        c_elements/element_cleanPopup.c
        c_elements/element_drainPopup.c
        c_elements/element_selfcheckPopup.c
        c_elements/element_otaWifiPopup.c
        c_elements/element_wifiPopup.c
        c_elements/element_splashPopup.c
        c_fonts/FilMachineFontIcons_15.c
        c_fonts/FilMachineFontIcons_20.c
        c_fonts/FilMachineFontIcons_30.c
        c_fonts/FilMachineFontIcons_40.c
        c_fonts/FilMachineFontIcons_50.c
        c_fonts/FilMachineFontIcons_60.c
        c_fonts/FilMachineFontIcons_100.c
        c_fonts/lv_font_montserrat_64.c
        c_fonts/font_air_americana_48.c
        c_fonts/font_decaying_felt_pen_48.c
        c_fonts/font_ds_digital_48.c
        c_fonts/font_evanescent_48.c
        c_fonts/font_nerdropol_lattice_48.c
        c_fonts/font_retrolight_48.c
        c_fonts/font_tropical_leaves_48.c
        c_fonts/font_wishful_melisande_48.c
        c_pages/page_splash.c
    )

    add_executable(filmachine_sim
        ${SIMULATOR_SOURCES}
        ${FILMACHINE_SOURCES}
    )

    target_include_directories(filmachine_sim PRIVATE
        ${CMAKE_SOURCE_DIR}/main/include
        ${CMAKE_SOURCE_DIR}/drivers/include
        ${CMAKE_SOURCE_DIR}/boards
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/stub
        ${CMAKE_SOURCE_DIR}/lvgl_config
        ${SDL2_INCLUDE_DIRS}
    )

    target_compile_definitions(filmachine_sim PRIVATE BOARD_SIMULATOR)

    target_link_libraries(filmachine_sim
        lvgl
        ${SDL2_LIBRARIES}
        m
        pthread
    )

    target_compile_options(filmachine_sim PRIVATE
        ${SDL2_CFLAGS_OTHER}
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-missing-field-initializers
    )

    # ─────────────────────────────────────────────
    # Test build — same sources but with test runner instead of src/main.c
    # ─────────────────────────────────────────────
    file(GLOB TEST_SOURCES tests/test_*.c)

    add_executable(filmachine_test
        ${TEST_SOURCES}
        stub/esp_stubs.c
        stub/fatfs_stubs.c
        stub/freertos_stubs.c
        ${FILMACHINE_SOURCES}
    )

    target_include_directories(filmachine_test PRIVATE
        ${CMAKE_SOURCE_DIR}/main/include
        ${CMAKE_SOURCE_DIR}/drivers/include
        ${CMAKE_SOURCE_DIR}/boards
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/stub
        ${CMAKE_SOURCE_DIR}/lvgl_config
        ${SDL2_INCLUDE_DIRS}
    )

    target_compile_definitions(filmachine_test PRIVATE
        BOARD_SIMULATOR
        SIM_RESOLUTION=800
        TEST_RESULTS_DIR="${CMAKE_SOURCE_DIR}/test_results"
    )

    target_link_libraries(filmachine_test
        lvgl
        ${SDL2_LIBRARIES}
        m
        pthread
    )

    target_compile_options(filmachine_test PRIVATE
        ${SDL2_CFLAGS_OTHER}
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-missing-field-initializers
    )
endif()
