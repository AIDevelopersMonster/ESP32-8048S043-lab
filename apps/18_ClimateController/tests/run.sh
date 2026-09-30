#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../.."
output="$(mktemp)"
trap 'rm -f "$output"' EXIT
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
  -I apps/18_ClimateController/tests/stubs -I apps/06_OTARecovery/idf/main \
  apps/18_ClimateController/tests/test_climate.c \
  apps/06_OTARecovery/idf/main/climate_logic.c \
  apps/06_OTARecovery/idf/main/climate_service.c -o "$output"
"$output"
for tick_ms in 1 10; do
  cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined \
    -DTEST_TICK_MS="$tick_ms" \
    -I apps/18_ClimateController/tests/stubs -I apps/06_OTARecovery/idf/main \
    apps/18_ClimateController/tests/test_modbus_rtu_io.c -o "$output"
  "$output"
done
