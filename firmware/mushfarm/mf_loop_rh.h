#pragma once

#include "mf_loop_common.h"

void mf_loop_rh_reset();
mf_loop_demand_t mf_loop_rh_tick(float dt_sec);
