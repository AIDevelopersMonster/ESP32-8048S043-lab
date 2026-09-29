#pragma once

#include <stddef.h>
#include <stdint.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Caller holds the bus mutex throughout the gap, TX and RX. At 9600 baud,
 * 10 ms exceeds 3.5 characters and leaves the automatic RS485 adapter time
 * to return to receive. The extra tick guarantees the minimum at tick edges. */
static inline void modbus_rtu_request_gap(void)
{
    vTaskDelay(pdMS_TO_TICKS(10) + 1);
}

/* UART may deliver a frame in fragments. Use one deadline for the whole
 * response, including after a partial read; do not renew the timeout. */
static inline size_t modbus_rtu_read_exact(uart_port_t port, uint8_t *out,
                                          size_t expected, TickType_t timeout)
{
    size_t got = 0;
    TickType_t deadline = xTaskGetTickCount() + timeout;
    while (got < expected) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(deadline - now) <= 0) break;
        int n = uart_read_bytes(port, out + got, expected - got, deadline - now);
        if (n > 0) got += (size_t)n;
    }
    return got;
}
