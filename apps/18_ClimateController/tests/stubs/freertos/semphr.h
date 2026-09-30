#pragma once
typedef void *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
int xSemaphoreTake(SemaphoreHandle_t s, unsigned delay);
int xSemaphoreGive(SemaphoreHandle_t s);
