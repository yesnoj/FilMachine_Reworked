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
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(FilMachine)

else()
    # ─────────────────────────────────────────────
    # PC Simulator build
    # ─────────────────────────────────────────────
    project(FilMachine_Simulator C)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)

    # ──────────────────────────────────────────────
    # SDL2
    # ──────────────────────────────────────────────
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    link_directories(${SDL2_LIBRARY_DIRS} /opt/homebrew/lib)

    # ──────────────────────────────────────────────
    # LVGL 9.2.2
    # ──────────────────────────────────────────────
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

    # ──────────────────────────────────────────────
    # Simulator entry point & stubs
    # ──────────────────────────────────────────────
    set(SIMULATOR_SOURCES
        src/main.c
        stub/esp_stubs.c
        stub/fatfs_stubs.c
        stub/freertos_stubs.c
    )

    # ──────────────────────────────────────────────
    # FilMachine sources — mirrors Git repo structure
    # ──────────────────────────────────────────────
    set(FILMACHINE_SOURCES
        # main/
        main/accessories.c
        main/mcp23017.c
        main/ds18b20.c
        main/pca9685.c
        main/ota_update.c

        # c_pages/
        c_pages/page_home.c
        c_pages/page_menu.c
        c_pages/page_processes.c
        c_pages/page_processDetail.c
        c_pages/page_stepDetail.c
        c_pages/page_settings.c
        c_pages/page_tools.c
        c_pages/page_checkup.c

        # c_elements/
        c_elements/element_process.c
        c_elements/element_step.c
        c_elements/element_filterPopup.c
        c_elements/element_messagePopup.c
        c_elements/element_rollerPopup.c
        c_elements/element_cleanPopup.c
        c_elements/element_drainPopup.c
        c_elements/element_selfcheckPopup.c
        c_elements/element_otaWifiPopup.c

        # c_fonts/
        c_fonts/FilMachineFontIcons_15.c
        c_fonts/FilMachineFontIcons_20.c
        c_fonts/FilMachineFontIcons_30.c
        c_fonts/FilMachineFontIcons_40.c
        c_fonts/FilMachineFontIcons_100.c

        # c_graphics/
        c_graphics/splash.c
    )

    add_executable(filmachine_sim
        ${SIMULATOR_SOURCES}
        ${FILMACHINE_SOURCES}
    )

    target_include_directories(filmachine_sim PRIVATE
        ${CMAKE_SOURCE_DIR}/main/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/stub
        ${CMAKE_SOURCE_DIR}/lvgl_config
        ${SDL2_INCLUDE_DIRS}
    )

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
        -Wno-unused-but-set-variable
        -Wno-missing-braces
    )

    if(APPLE)
        target_link_libraries(filmachine_sim "-framework Cocoa")
    endif()

    # ──────────────────────────────────────────────
    # Ensure sd/ folder with config file exists in
    # build directory (simulator needs FilMachine.cfg)
    # ──────────────────────────────────────────────
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/sd")

    if(NOT EXISTS "${CMAKE_BINARY_DIR}/sd/FilMachine.cfg")
        # Priority 1: copy from source tree
        if(EXISTS "${CMAKE_SOURCE_DIR}/sd/FilMachine.cfg")
            file(COPY "${CMAKE_SOURCE_DIR}/sd/" DESTINATION "${CMAKE_BINARY_DIR}/sd")
            message(STATUS "Copied config files: ${CMAKE_SOURCE_DIR}/sd/ → build/sd/")
        else()
            # Priority 2: generate via Python script
            find_package(Python3 COMPONENTS Interpreter QUIET)
            if(Python3_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/scripts/genFilMachineCFG.py")
                execute_process(
                    COMMAND ${Python3_EXECUTABLE}
                            "${CMAKE_SOURCE_DIR}/scripts/genFilMachineCFG.py"
                            --realistic
                            --output "${CMAKE_BINARY_DIR}/sd"
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                )
                message(STATUS "Generated config via genFilMachineCFG.py → build/sd/")
            else()
                message(WARNING "No config found! Run manually:\n"
                        "  python3 scripts/genFilMachineCFG.py --realistic --output build/sd/")
            endif()
        endif()
    endif()

    # ──────────────────────────────────────────────
    # Automated Test Runner
    # ──────────────────────────────────────────────
    set(TEST_SOURCES
        tests/test_runner.c
        tests/test_helpers.c
        tests/test_navigation.c
        tests/test_processes.c
        tests/test_process_crud.c
        tests/test_steps.c
        tests/test_step_crud.c
        tests/test_execution.c
        tests/test_persistence.c
        tests/test_settings.c
        tests/test_filter.c
        tests/test_tools.c
        tests/test_edge_cases.c
        tests/test_utilities.c
        tests/test_destroy_and_lifecycle.c
    )

    add_executable(filmachine_test
        ${TEST_SOURCES}
        # Stubs (same as simulator, but test_runner.c replaces src/main.c)
        stub/esp_stubs.c
        stub/fatfs_stubs.c
        stub/freertos_stubs.c
        # All FilMachine application sources
        ${FILMACHINE_SOURCES}
    )

    target_include_directories(filmachine_test PRIVATE
        ${CMAKE_SOURCE_DIR}/main/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/stub
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/lvgl_config
        ${SDL2_INCLUDE_DIRS}
    )

    target_link_libraries(filmachine_test
        lvgl
        ${SDL2_LIBRARIES}
        m
        pthread
    )

    target_compile_definitions(filmachine_test PRIVATE
        TEST_RESULTS_DIR="${CMAKE_SOURCE_DIR}/test_results"
    )

    target_compile_options(filmachine_test PRIVATE
        ${SDL2_CFLAGS_OTHER}
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-missing-braces
    )

    if(APPLE)
        target_link_libraries(filmachine_test "-framework Cocoa")
    endif()

endif()  # ESP_PLATFORM
