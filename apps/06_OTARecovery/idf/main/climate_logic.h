#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t t_min, t_max, t_hyst; /* tenths of degrees C */
    int16_t rh_min, rh_max, rh_hyst; /* tenths of %RH */
} climate_config_t;

climate_config_t climate_defaults(void);
bool climate_config_valid(const climate_config_t *c);
/* Strict comparisons: equality retains the previous state. */
void climate_evaluate(const climate_config_t *c, int16_t t, uint16_t rh,
                      bool demand[4]);
