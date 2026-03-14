/**
 * main.c — FilMachine PC Simulator Entry Point
 *
 * This file REPLACES FilMachine.c (the ESP32 firmware entry point).
 * It defines the same global variables and provides stubs for
 * hardware-specific functions that were in FilMachine.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "FilMachine.h"
#include "src/indev/lv_indev_private.h"   /* access gesture_limit / gesture_min_velocity */

/* ═══════════════════════════════════════════════
 * Global variables (originally in FilMachine.c)
 * ═══════════════════════════════════════════════ */
struct gui_components   gui;
struct sys_components   sys;
bool                    stopMotorManTask = false;
uint8_t                 initErrors = 0;

/* ═══════════════════════════════════════════════
 * Stubs for functions defined in FilMachine.c
 * ═══════════════════════════════════════════════ */
void stopMotorTask(void) {
    stopMotorManTask = true;
    printf("[SIM] Motor task stopped\n");
}

void runMotorTask(void) {
    printf("[SIM] Motor task started (stub — not running in simulator)\n");
    stopMotorManTask = false;
}

/* ═══════════════════════════════════════════════
 * Weak stubs for hardware functions.
 * If accessories.c already defines them (even with empty #if 0 bodies),
 * the linker will prefer that version. Otherwise these are used.
 * ═══════════════════════════════════════════════ */
__attribute__((weak)) void stopMotor(uint8_t pin1, uint8_t pin2) {
    printf("[SIM] Motor stop\n");
}
__attribute__((weak)) void runMotorFW(uint8_t pin1, uint8_t pin2) {
    printf("[SIM] Motor forward\n");
}
__attribute__((weak)) void runMotorRV(uint8_t pin1, uint8_t pin2) {
    printf("[SIM] Motor reverse\n");
}
__attribute__((weak)) void setMotorSpeed(uint8_t pin, uint8_t spd) { }
__attribute__((weak)) void setMotorSpeedUp(uint8_t pin, uint8_t spd) { }
__attribute__((weak)) void setMotorSpeedDown(uint8_t pin, uint8_t spd) { }
__attribute__((weak)) void enableMotor(uint8_t pin) { }
__attribute__((weak)) void testPin(uint8_t pin) { }
__attribute__((weak)) void initializeRelayPins(void) { }
__attribute__((weak)) void initializeMotorPins(void) { }
__attribute__((weak)) void initializeTemperatureSensor(void) { }
__attribute__((weak)) void printTemperature(float temp) { }
__attribute__((weak)) void init_Pins_and_Buses(void) { }
__attribute__((weak)) void initMCP23017Pins(void) { }
__attribute__((weak)) void pwmLedTest(void) { }
__attribute__((weak)) void cleanRelayManager(uint8_t a, uint8_t b, uint8_t c, bool d) { }
__attribute__((weak)) void sendValueToRelay(uint8_t a, uint8_t b, bool c) { }
__attribute__((weak)) void rebootBoard(void) {
    printf("[SIM] Reboot requested — ignoring in simulator\n");
}

/* ═══════════════════════════════════════════════
 * Time helper
 * ═══════════════════════════════════════════════ */
static uint32_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/* ═══════════════════════════════════════════════
 * Main entry point
 * ═══════════════════════════════════════════════ */
int main(int argc, char *argv[]) {

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   FilMachine Simulator                   ║\n");
    printf("║   Resolution: %dx%d                    ║\n", LCD_H_RES, LCD_V_RES);
    printf("║   Mouse = Touch | Close window to quit   ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    srand((unsigned int)time(NULL));

    /* Initialize globals (zeroes out gui and sys structs) */
    initGlobals();

    /* Initialize LVGL */
    lv_init();

    /* Create SDL display (LVGL 9 built-in SDL driver) */
    lv_display_t *display = lv_sdl_window_create(LCD_H_RES, LCD_V_RES);
    if (!display) {
        fprintf(stderr, "ERROR: Failed to create SDL display!\n");
        return 1;
    }

    /* Create SDL mouse input (simulates touchscreen) */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    if (!mouse) {
        fprintf(stderr, "ERROR: Failed to create SDL mouse input!\n");
        return 1;
    }

    /* Lower gesture thresholds so short/slow swipes are detected more reliably.
       Defaults: gesture_limit=50, gesture_min_velocity=3 */
    mouse->gesture_limit        = 30;   /* was 50 — pixels of movement to trigger GESTURE */
    mouse->gesture_min_velocity = 2;    /* was 3  — min per-frame velocity to keep accumulating */

    /* Create FreeRTOS queues (stub implementation) */
    sys.sysActionQ = xQueueCreate(16, sizeof(uint16_t));
    sys.motorActionQ = xQueueCreate(16, sizeof(uint16_t));

    /* Create shared keyboard */
    create_keyboard();

    /* Launch the home page — the real FilMachine GUI! */
    homePage();

    /* Optionally load saved configuration */
    readConfigFile(FILENAME_SAVE, false);

    printf("[SIM] GUI initialized — entering main loop\n");
    printf("[SIM] Use mouse to interact, close window to quit\n");

    /* Main loop */
    uint32_t last_tick = get_time_ms();

    while (1) {
        uint32_t now = get_time_ms();
        uint32_t elapsed = now - last_tick;
        last_tick = now;

        lv_tick_inc(elapsed);
        lv_timer_handler();

        usleep(5000); /* ~5ms */
    }

    return 0;
}
