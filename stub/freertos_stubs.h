/**
 * freertos_stubs.h — Minimal FreeRTOS API stubs for PC simulator
 */

#ifndef FREERTOS_STUBS_H
#define FREERTOS_STUBS_H

#include "esp_stubs.h"  /* Ensures GPIO_NUM_xx, ESP_LOG, etc. are available */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ─────────────────────────────────────────────
 * Basic types
 * ───────────────────────────────────────────── */
typedef void* QueueHandle_t;
typedef void* TaskHandle_t;
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;

#define pdTRUE   1
#define pdFALSE  0
#define pdPASS   1
#define pdFAIL   0
#define portMAX_DELAY 0xFFFFFFFF
#define tskNO_AFFINITY 0x7FFFFFFF

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#endif

/* ─────────────────────────────────────────────
 * Queue — simple circular buffer stub
 * ───────────────────────────────────────────── */
typedef struct {
    uint8_t *buffer;
    uint16_t itemSize;
    uint16_t maxItems;
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} SimQueue;

static inline QueueHandle_t xQueueCreate(uint16_t length, uint16_t itemSize) {
    SimQueue *q = (SimQueue*)calloc(1, sizeof(SimQueue));
    if (!q) return NULL;
    q->buffer = (uint8_t*)calloc(length, itemSize);
    q->itemSize = itemSize;
    q->maxItems = length;
    return (QueueHandle_t)q;
}

static inline BaseType_t xQueueSend(QueueHandle_t handle, const void *item, TickType_t wait) {
    SimQueue *q = (SimQueue*)handle;
    if (!q || q->count >= q->maxItems) return pdFAIL;
    memcpy(q->buffer + (q->tail * q->itemSize), item, q->itemSize);
    q->tail = (q->tail + 1) % q->maxItems;
    q->count++;
    return pdPASS;
}

static inline BaseType_t xQueueReceive(QueueHandle_t handle, void *item, TickType_t wait) {
    SimQueue *q = (SimQueue*)handle;
    if (!q || q->count == 0) return pdFAIL;
    memcpy(item, q->buffer + (q->head * q->itemSize), q->itemSize);
    q->head = (q->head + 1) % q->maxItems;
    q->count--;
    return pdPASS;
}

/* ─────────────────────────────────────────────
 * Task — stubs (tasks don't actually run in sim)
 * ───────────────────────────────────────────── */
static inline BaseType_t xTaskCreatePinnedToCore(
    void (*taskFunc)(void*), const char *name, uint32_t stackSize,
    void *params, int priority, TaskHandle_t *handle, int coreId) {
    printf("[FreeRTOS STUB] Task '%s' created (not running in simulator)\n", name);
    return pdPASS;
}

static inline void vTaskDelay(TickType_t ticks) { /* No-op */ }
static inline void vTaskDelete(TaskHandle_t handle) { }

#endif /* FREERTOS_STUBS_H */
