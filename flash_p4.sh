#!/bin/bash
# flash_p4.sh — Build and flash FilMachine for ESP32-P4 (JC4880P433)
# Builds directly from FilMachine_Simulator_v2 (no separate firmware repo)
set -e

IDF_V55="$HOME/esp/esp-idf-v5.5/export.sh"
PORT="/dev/cu.usbmodem101"

# ── Check ESP-IDF 5.5 ──
if [ ! -f "$IDF_V55" ]; then
    echo "❌ ESP-IDF v5.5 not found at $IDF_V55"
    echo "   ESP32-P4 requires ESP-IDF 5.5.x or later."
    exit 1
fi

echo "=== Sourcing ESP-IDF v5.5 ==="
source "$IDF_V55"

# ── Ensure we're in the Simulator directory ──
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ── Set target if needed ──
CURRENT_TARGET=""
if [ -f "sdkconfig" ]; then
    CURRENT_TARGET=$(grep -oP 'CONFIG_IDF_TARGET="\K[^"]+' sdkconfig 2>/dev/null || true)
fi

if [ "$CURRENT_TARGET" != "esp32p4" ]; then
    echo "=== Setting target to ESP32-P4 (fullclean + set-target) ==="
    idf.py fullclean 2>/dev/null || true
    idf.py set-target esp32p4
fi

# ── Patch sdkconfig for P4 (settings that sdkconfig.defaults.esp32p4 can't apply) ──
echo "=== Patching sdkconfig for P4 board ==="
SDKCONFIG="sdkconfig"

# Function to set a config value
set_config() {
    local key="$1"
    local value="$2"
    if grep -q "^${key}=" "$SDKCONFIG" 2>/dev/null; then
        sed -i '' "s|^${key}=.*|${key}=${value}|" "$SDKCONFIG"
    elif grep -q "^# ${key} is not set" "$SDKCONFIG" 2>/dev/null; then
        sed -i '' "s|^# ${key} is not set|${key}=${value}|" "$SDKCONFIG"
    else
        echo "${key}=${value}" >> "$SDKCONFIG"
    fi
}

# Flash size: 16MB
set_config CONFIG_ESPTOOLPY_FLASHSIZE_16MB y
set_config CONFIG_ESPTOOLPY_FLASHSIZE '"16MB"'
# Disable smaller flash size options
sed -i '' 's/^CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y/# CONFIG_ESPTOOLPY_FLASHSIZE_2MB is not set/' "$SDKCONFIG" 2>/dev/null || true
sed -i '' 's/^CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y/# CONFIG_ESPTOOLPY_FLASHSIZE_4MB is not set/' "$SDKCONFIG" 2>/dev/null || true

# PSRAM (32 MB HEX mode — required for MIPI-DSI frame buffer)
set_config CONFIG_SPIRAM y
set_config CONFIG_SPIRAM_MODE_HEX y
set_config CONFIG_SPIRAM_SPEED_80M y
set_config CONFIG_SPIRAM_SPEED 80
set_config CONFIG_SPIRAM_BOOT_INIT y
# Disable 20M (kconfig only offers 20M and 80M for HEX mode in ESP-IDF 5.5)
sed -i '' 's/^CONFIG_SPIRAM_SPEED_20M=y/# CONFIG_SPIRAM_SPEED_20M is not set/' "$SDKCONFIG" 2>/dev/null || true

# PSRAM allocation: use heap_caps (same as RetroESP32-P4 reference project)
set_config CONFIG_SPIRAM_USE_CAPS_ALLOC y
# Disable conflicting SPIRAM_USE options
sed -i '' 's/^CONFIG_SPIRAM_USE_MALLOC=y/# CONFIG_SPIRAM_USE_MALLOC is not set/' "$SDKCONFIG" 2>/dev/null || true

# Custom partition table
set_config CONFIG_PARTITION_TABLE_CUSTOM y
set_config CONFIG_PARTITION_TABLE_FILENAME '"partitions.csv"'
sed -i '' 's/^CONFIG_PARTITION_TABLE_SINGLE_APP=y/# CONFIG_PARTITION_TABLE_SINGLE_APP is not set/' "$SDKCONFIG" 2>/dev/null || true

# Main task stack — 3584 default is too small for P4 (LVGL + FatFS + display init)
set_config CONFIG_ESP_MAIN_TASK_STACK_SIZE 8192

# Task watchdog — LVGL full-frame rendering at 800x480 blocks CPU0 for extended periods
set_config CONFIG_ESP_TASK_WDT_TIMEOUT_S 30
set_config CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0 n
sed -i '' 's/^CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0=y/# CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0 is not set/' "$SDKCONFIG" 2>/dev/null || true

# LVGL memory: use standard C malloc/free (system heap = 32MB PSRAM)
# CONFIG_LV_MEM_CUSTOM was LVGL 8; LVGL 9 uses CONFIG_LV_USE_CLIB_MALLOC
set_config CONFIG_LV_USE_CLIB_MALLOC y
set_config CONFIG_LV_USE_CLIB_STRING y
set_config CONFIG_LV_USE_CLIB_SPRINTF y
# Disable LVGL builtin allocator (mutually exclusive choice)
sed -i '' 's/^CONFIG_LV_USE_BUILTIN_MALLOC=y/# CONFIG_LV_USE_BUILTIN_MALLOC is not set/' "$SDKCONFIG" 2>/dev/null || true
sed -i '' 's/^CONFIG_LV_USE_BUILTIN_STRING=y/# CONFIG_LV_USE_BUILTIN_STRING is not set/' "$SDKCONFIG" 2>/dev/null || true
sed -i '' 's/^CONFIG_LV_USE_BUILTIN_SPRINTF=y/# CONFIG_LV_USE_BUILTIN_SPRINTF is not set/' "$SDKCONFIG" 2>/dev/null || true

# LVGL dark theme (FilMachine UI expects black background)
set_config CONFIG_LV_THEME_DEFAULT_DARK y

# FatFS Long File Names — required for "FilMachine.cfg" (10-char name exceeds 8.3)
set_config CONFIG_FATFS_LFN_HEAP y
set_config CONFIG_FATFS_MAX_LFN 127
sed -i '' 's/^CONFIG_FATFS_LFN_NONE=y/# CONFIG_FATFS_LFN_NONE is not set/' "$SDKCONFIG" 2>/dev/null || true
sed -i '' 's/^CONFIG_FATFS_LFN_STACK=y/# CONFIG_FATFS_LFN_STACK is not set/' "$SDKCONFIG" 2>/dev/null || true

# LVGL Montserrat fonts (12-48)
for size in 12 14 16 18 20 22 24 26 28 30 32 36 40 48; do
    set_config "CONFIG_LV_FONT_MONTSERRAT_${size}" y
done

# HTTP Server WebSocket support (required for ws_server.c)
set_config CONFIG_HTTPD_WS_SUPPORT y

# ESP-Hosted: WiFi via ESP32-C6 companion chip over SDIO
set_config CONFIG_ESP_WIFI_REMOTE_ENABLED y
set_config CONFIG_SLAVE_IDF_TARGET_ESP32C6 y
set_config CONFIG_ESP_HOSTED_CP_TARGET_ESP32C6 y
# lwIP tuning for SDIO bridge throughput
set_config CONFIG_LWIP_TCP_SND_BUF_DEFAULT 65534
set_config CONFIG_LWIP_TCP_WND_DEFAULT 65534
set_config CONFIG_LWIP_TCP_RECVMBOX_SIZE 64
set_config CONFIG_LWIP_UDP_RECVMBOX_SIZE 64
set_config CONFIG_LWIP_TCPIP_RECVMBOX_SIZE 64
set_config CONFIG_LWIP_TCP_SACK_OUT y

# Force kconfig to re-resolve dependencies (ensures PSRAM 200M sticks)
echo "=== Reconfiguring (resolving kconfig dependencies) ==="
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4880P433" reconfigure

echo "=== Building with BOARD_JC4880P433 flag ==="
idf.py -D CMAKE_C_FLAGS="-DBOARD_JC4880P433" build

# ── Detect port if default doesn't exist ──
if [ ! -e "$PORT" ]; then
    echo "⚠️  Default port $PORT not found. Scanning..."
    for p in /dev/cu.usbmodem* /dev/cu.usbserial* /dev/cu.wchusbserial*; do
        [ -e "$p" ] && PORT="$p" && break
    done
    if [ ! -e "$PORT" ]; then
        echo "❌ No USB serial port found. Is the board plugged in?"
        exit 1
    fi
fi

echo "=== Flashing to $PORT ==="
idf.py -p "$PORT" flash

echo ""
echo "✅ Flash complete! Opening monitor (Ctrl+] to exit)..."
idf.py -p "$PORT" monitor
