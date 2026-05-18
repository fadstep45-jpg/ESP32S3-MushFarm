#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mf_actuators.h"

bool mf_actuator_test_blocks_climate();
bool mf_actuator_test_start(mf_actuator_t which, float percent, uint32_t timeout_s);
void mf_actuator_test_poll();
