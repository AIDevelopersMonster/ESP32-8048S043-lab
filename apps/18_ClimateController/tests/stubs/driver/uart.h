#pragma once
#include <stddef.h>
#include "freertos/FreeRTOS.h"
typedef int uart_port_t;
int uart_read_bytes(uart_port_t port, void *out, size_t length, TickType_t timeout);
