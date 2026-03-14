cmake_minimum_required(VERSION 3.14)
project(FilMachine_Simulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────────
# SDL2
# ──────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

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

link_directories(/opt/homebrew/lib)
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
    link_directories(/opt/homebrew/lib)
    target_link_libraries(filmachine_sim "-framework Cocoa")
endif()
