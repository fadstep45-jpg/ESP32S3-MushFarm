#include "mf_climate.h"
#include "mf_recipe.h"
#include "mf_sensor_scd41.h"
#include "mf_actuator_pwm.h"
#include "mf_fsm.h"
#include "esp_log.h"

static const char *TAG = "mf_climate";

void mf_climate_tick(void)
{
    if (mf_fsm_state() != MF_STATE_ACTIVE_RUN) {
        (void)mf_actuator_pwm_set_percent(MF_ACT_FAN, 0);
        (void)mf_actuator_pwm_set_percent(MF_ACT_HUMIDIFIER, 0);
        return;
    }
    float target = mf_recipe_rh_target_percent();
    float rh = mf_sensor_scd41_rh_percent();
    float err = target - rh;
    /* P-only stub: drive humidifier proportional to positive RH error */
    float hum = err > 0 ? err * 5.0f : 0;
    if (hum > 80) {
        hum = 80;
    }
    float fan = mf_sensor_scd41_co2_ppm() > 1500 ? 40.0f : 15.0f;
    (void)mf_actuator_pwm_set_percent(MF_ACT_HUMIDIFIER, hum);
    (void)mf_actuator_pwm_set_percent(MF_ACT_FAN, fan);
    ESP_LOGD(TAG, "RH %.1f -> %.1f hum=%.0f fan=%.0f", rh, target, hum, fan);
}
