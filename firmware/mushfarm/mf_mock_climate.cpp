#include "mf_mock_climate.h"
#include "mf_config.h"
#include "mf_control_profile.h"

static mf_mock_climate_scenario_t s_scenario =
    (mf_mock_climate_scenario_t)MF_MOCK_CLIMATE_SCENARIO;

void mf_mock_climate_set_scenario(mf_mock_climate_scenario_t scenario) {
    s_scenario = scenario;
}

mf_mock_climate_scenario_t mf_mock_climate_get_scenario() {
    return s_scenario;
}

void mf_mock_climate_apply_scd41(float *co2, float *rh, float *air_c) {
    if (!co2 || !rh || !air_c) return;
    mf_mock_climate_scenario_t sc = s_scenario;
    if (sc == MF_MOCK_SCENARIO_AUTO) {
        return;
    }
    const mf_control_profile_t *p = mf_control_profile_current();
    switch (sc) {
    case MF_MOCK_SCENARIO_RH_LOW_CO2_OK:
        *rh = p->rh_target_percent - 20.0f;
        *co2 = p->co2_target_ppm - 200.0f;
        *air_c = p->air_temp_target_c;
        break;
    case MF_MOCK_SCENARIO_CO2_HIGH:
        *rh = p->rh_target_percent;
        *co2 = p->co2_target_ppm + p->co2_hysteresis_ppm + 400.0f;
        *air_c = p->air_temp_target_c;
        break;
    case MF_MOCK_SCENARIO_CO2_CRIT:
        *co2 = p->co2_emergency_absolute_ppm + 200.0f;
        *rh = p->rh_target_percent;
        *air_c = p->air_temp_target_c;
        break;
    case MF_MOCK_SCENARIO_RH_MAX:
        *rh = p->rh_max_crit_percent + 0.5f;
        *co2 = p->co2_target_ppm;
        *air_c = p->air_temp_target_c;
        break;
    case MF_MOCK_SCENARIO_CONDENSATE:
        *air_c = p->air_temp_target_c;
        *co2 = p->co2_target_ppm;
        *rh = p->rh_target_percent;
        break;
    case MF_MOCK_SCENARIO_DISCONNECT:
    default:
        break;
    }
}

void mf_mock_climate_apply_mlx(float *obj_c, float air_c) {
    if (!obj_c) return;
    if (s_scenario == MF_MOCK_SCENARIO_CONDENSATE) {
        const mf_control_profile_t *p = mf_control_profile_current();
        *obj_c = air_c + p->max_substrate_air_delta_c + 2.0f;
    } else if (s_scenario != MF_MOCK_SCENARIO_AUTO &&
               s_scenario != MF_MOCK_SCENARIO_DISCONNECT) {
        *obj_c = mf_control_profile_current()->substrate_temp_target_c;
    }
}
