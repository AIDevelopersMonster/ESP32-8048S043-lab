#pragma once
#include <stdint.h>
typedef uint32_t TickType_t;
#ifndef TEST_TICK_MS
#define TEST_TICK_MS 10
#endif
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / TEST_TICK_MS))
#define portMAX_DELAY 0
