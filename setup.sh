#!/bin/bash
# setup.sh — Restructure FilMachine Simulator to mirror Git repo layout
# Run from ~/Desktop/

set -e

REPO="/Users/francescoprochilo/Documents/GitHub/FilMachine_Pete"
OLD=~/Desktop/FilMachine_Simulator
NEW=~/Desktop/FilMachine_Simulator_v2

echo "=== Setting up restructured FilMachine Simulator ==="

# 1. Create directory structure matching Git repo
echo "[1/6] Creating directory structure..."
mkdir -p "$NEW/main/include"
mkdir -p "$NEW/c_pages"
mkdir -p "$NEW/c_elements"
mkdir -p "$NEW/c_fonts"
mkdir -p "$NEW/c_graphics"
mkdir -p "$NEW/src"
mkdir -p "$NEW/stub/driver"
mkdir -p "$NEW/stub/freertos"
mkdir -p "$NEW/stub/misc"
mkdir -p "$NEW/lvgl_config"

# 2. Copy source files DIRECTLY from Git repo (single source of truth)
echo "[2/6] Copying source files from Git repo..."
cp "$REPO/main/include/FilMachine.h"   "$NEW/main/include/"
cp "$REPO/main/accessories.c"          "$NEW/main/"
cp "$REPO/c_pages/"*.c                 "$NEW/c_pages/"
cp "$REPO/c_elements/"*.c              "$NEW/c_elements/"
cp "$REPO/c_graphics/splash.c"         "$NEW/c_graphics/"

# 3. Copy regenerated fonts from old simulator (these are the fixed ones)
echo "[3/6] Copying regenerated fonts..."
cp "$OLD/src/FilMachineFontIcons_"*.c  "$NEW/c_fonts/"

# 4. Copy simulator-specific files from old setup
echo "[4/6] Copying simulator infrastructure..."
cp "$OLD/src/main.c"                   "$NEW/src/"
cp "$OLD/stub/esp_stubs.h"            "$NEW/stub/"
cp "$OLD/stub/esp_stubs.c"            "$NEW/stub/"
cp "$OLD/stub/fatfs_stubs.h"          "$NEW/stub/"
cp "$OLD/stub/fatfs_stubs.c"          "$NEW/stub/"
cp "$OLD/stub/freertos_stubs.h"       "$NEW/stub/"
cp "$OLD/stub/freertos_stubs.c"       "$NEW/stub/"
cp "$OLD/lvgl_config/lv_conf.h"       "$NEW/lvgl_config/"

# 5. Copy stub redirect headers
echo "[5/6] Creating stub redirect headers..."
for f in esp_err.h esp_log.h esp_timer.h esp_heap_caps.h esp_lcd_panel_dev.h esp_lcd_panel_ops.h esp_lcd_ili9488.h esp_lcd_touch.h esp_lcd_touch_ft5x06.h esp_vfs_fat.h sdmmc_cmd.h; do
    echo '#include "esp_stubs.h"' > "$NEW/stub/$f"
done
echo '#include "fatfs_stubs.h"' > "$NEW/stub/ff.h"
echo '#include "esp_stubs.h"' > "$NEW/stub/driver/sdmmc_host.h"
echo '#include "esp_stubs.h"' > "$NEW/stub/driver/i2c.h"
echo '#include "esp_stubs.h"' > "$NEW/stub/driver/gpio.h"
echo '#include "freertos_stubs.h"' > "$NEW/stub/freertos/FreeRTOS.h"
echo '#include "freertos_stubs.h"' > "$NEW/stub/freertos/task.h"
echo '#include "lvgl.h"' > "$NEW/stub/misc/lv_color.h"

# 6. Copy LVGL (reuse existing clone to save time)
echo "[6/6] Copying LVGL..."
if [ -d "$OLD/lvgl" ]; then
    cp -r "$OLD/lvgl" "$NEW/lvgl"
else
    echo "LVGL will be cloned on first build"
fi

# Copy .gitignore
cat > "$NEW/.gitignore" << 'GITIGNORE'
build/
lvgl/
sim_data/
.DS_Store
*.o
*.d
GITIGNORE

echo ""
echo "=== Done! ==="
echo ""
echo "Structure:"
echo "  FilMachine_Simulator_v2/"
echo "  ├── main/                  ← same as Git repo"
echo "  │   ├── include/"
echo "  │   │   └── FilMachine.h"
echo "  │   └── accessories.c"
echo "  ├── c_pages/               ← same as Git repo"
echo "  │   ├── page_home.c"
echo "  │   ├── page_processes.c"
echo "  │   └── ..."
echo "  ├── c_elements/            ← same as Git repo"
echo "  │   ├── element_process.c"
echo "  │   └── ..."
echo "  ├── c_fonts/               ← same as Git repo"
echo "  ├── c_graphics/            ← same as Git repo"
echo "  ├── src/main.c             ← simulator only"
echo "  ├── stub/                  ← simulator only"
echo "  ├── lvgl/                  ← simulator only"
echo "  └── CMakeLists.txt         ← simulator only"
echo ""
echo "To build:"
echo "  cd ~/Desktop/FilMachine_Simulator_v2"
echo "  mkdir build && cd build"
echo "  cmake .. -DCMAKE_EXE_LINKER_FLAGS=\"-L/opt/homebrew/lib\""
echo "  make -j\$(sysctl -n hw.ncpu)"
echo "  ./filmachine_sim"
echo ""
echo "To sync back to Git repo:"
echo "  cp -r main/ c_pages/ c_elements/ c_fonts/ c_graphics/ \\"
echo "     $REPO/"
