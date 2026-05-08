#include "mf_climate.h"
#include "mf_actuators.h"
#include "mf_fsm.h"
#include "mf_recipe.h"
#include "mf_sensor_scd41.h"
#include "mf_log.h"

void mf_climate_tick() {
    mf_runtime_state_t st = mf_fsm_state();
    if (st != MF_STATE_ACTIVE_RUN && st != MF_STATE_DEGRADED_RUN) {
        mf_actuators_set_percent(MF_ACT_FAN, 0);
        mf_actuators_set_percent(MF_ACT_HUMIDIFIER, 0);
        return;
    }
    float target = mf_recipe_rh_target_percent();
    float rh = mf_scd41_rh_percent();
    float err = target - rh;

    // P-only stub: humidifier proportional to positive RH error.
    float hum = err > 0 ? err * 5.0f : 0.0f;
    if (hum > 80.0f) hum = 80.0f;
    float fan = mf_scd41_co2_ppm() > 1500.0f ? 40.0f : 15.0f;

    mf_actuators_set_percent(MF_ACT_HUMIDIFIER, hum);
    mf_actuators_set_percent(MF_ACT_FAN, fan);
}
