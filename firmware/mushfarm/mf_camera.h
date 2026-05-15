#pragma once

#include <stdbool.h>

// Camera subsystem (stub).
//
// The target board ships with a 24-pin FFC camera module (OV2640 /
// OV3660 / OV5640 — final model TBD). The real driver and DVP/SCCB
// pin assignment land in a dedicated sprint. Today this module is a
// no-op so that the rest of the firmware can already reference the
// camera lifecycle without depending on the choice of sensor.
//
// Gated by MF_CAMERA_ENABLE in mf_config.h.

bool mf_camera_init();

void mf_camera_poll();

const char *mf_camera_status_str();
