#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "modbus_rtu_io.h"

static TickType_t clock_ticks, deadline, delayed;
static size_t available, delivered, chunk;
static unsigned calls;
static const uint8_t frame[8] = {0x10, 5, 0, 0, 0, 0, 0xce, 0x8b};
TickType_t xTaskGetTickCount(void) { return clock_ticks; }
void vTaskDelay(TickType_t ticks) { delayed = ticks; clock_ticks += ticks; }
int uart_read_bytes(uart_port_t port, void *out, size_t length, TickType_t timeout)
{
    assert(port == 1);
    assert(timeout == (TickType_t)(deadline - clock_ticks));
    ++calls;
    if (delivered == available) { clock_ticks += timeout; return 0; }
    size_t n = available - delivered;
    if (n > chunk) n = chunk;
    if (n > length) n = length;
    memcpy(out, frame + delivered, n);
    delivered += n;
    ++clock_ticks;
    return (int)n;
}
static void response(size_t bytes, size_t fragment, TickType_t start)
{
    uint8_t out[8] = {0};
    clock_ticks = start; deadline = start + 25;
    available = bytes; delivered = calls = 0; chunk = fragment;
    assert(modbus_rtu_read_exact(1, out, sizeof(out), 25) == bytes);
    assert(!memcmp(out, frame, bytes));
    if (bytes < sizeof(out)) assert(clock_ticks == deadline);
    if (fragment == 1 && bytes == 8) assert(calls == 8);
}
int main(void)
{
    modbus_rtu_request_gap();
    /* A delay can start just before a tick: subtract one tick for the
     * guaranteed lower bound rather than assuming full tick durations. */
    assert((delayed - 1) * TEST_TICK_MS >= 10);
    response(8, 8, 0);
    response(8, 1, 0);
    response(3, 1, 0);
    response(0, 1, 0);
    response(8, 1, UINT32_MAX - 3);
    response(3, 1, UINT32_MAX - 3);
    puts("Modbus gap, fragmented RX, deadline and tick wrap: PASS");
}
