#pragma once

#include <stdint.h>

typedef enum mf_mock_climate_scenario_t {
    MF_MOCK_SCENARIO_AUTO = 0,
    MF_MOCK_SCENARIO_RH_LOW_CO2_OK,
    MF_MOCK_SCENARIO_CO2_HIGH,
    MF_MOCK_SCENARIO_CO2_CRIT,
    MF_MOCK_SCENARIO_RH_MAX,
    MF_MOCK_SCENARIO_CONDENSATE,
    MF_MOCK_SCENARIO_DISCONNECT,
} mf_mock_climate_scenario_t;

void mf_mock_climate_set_scenario(mf_mock_climate_scenario_t scenario);
mf_mock_climate_scenario_t mf_mock_climate_get_scenario();
void mf_mock_climate_apply_scd41(float *co2, float *rh, float *air_c);
void mf_mock_climate_apply_mlx(float *obj_c, float air_c);
