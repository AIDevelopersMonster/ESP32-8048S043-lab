#include "climate_logic.h"

climate_config_t climate_defaults(void)
{
    return (climate_config_t){250, 300, 5, 450, 600, 20};
}

bool climate_config_valid(const climate_config_t *c)
{
    return c && c->t_min >= -400 && c->t_max <= 1250 &&
        c->rh_min >= 0 && c->rh_max <= 1000 &&
        c->t_hyst > 0 && c->rh_hyst > 0 &&
        c->t_min + c->t_hyst < c->t_max - c->t_hyst &&
        c->rh_min + c->rh_hyst < c->rh_max - c->rh_hyst;
}

static void pair(int value, int low, int high, int hyst, bool *a, bool *b)
{
    if (value > low + hyst) *a = false;
    if (value < high - hyst) *b = false;
    if (value < low) { *b = false; *a = true; }
    if (value > high) { *a = false; *b = true; }
    if (*a && *b) { *a = false; *b = false; }
}

void climate_evaluate(const climate_config_t *c, int16_t t, uint16_t rh, bool d[4])
{
    if (!climate_config_valid(c) || t < -400 || t > 1250 || rh > 1000) {
        for (unsigned i = 0; i < 4; ++i) d[i] = false;
        return;
    }
    pair(t, c->t_min, c->t_max, c->t_hyst, &d[0], &d[1]);
    pair(rh, c->rh_min, c->rh_max, c->rh_hyst, &d[2], &d[3]);
}
