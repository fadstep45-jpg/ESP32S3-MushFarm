#pragma once

#include <stdint.h>
#include <stdbool.h>

void mf_water_init();
void mf_water_poll();

bool mf_water_present();
bool mf_water_ok(int64_t *stale_age_ms_out);

// Test/service hook: force the reported water level regardless of the
// actual GPIO or mock generator. `level` semantics:
//   -1 — disable override, fall back to real / mock pipeline (default)
//    0 — force water absent (LOW)
//    1 — force water present (HIGH)
// Used by the mushfarm_tests sketch and by service-mode fault injection.
void mf_water_set_simulated_present(int level);
