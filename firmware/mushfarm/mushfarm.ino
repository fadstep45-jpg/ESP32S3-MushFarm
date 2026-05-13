// MushFarm — ESP32-S3 mushroom farm controller (Arduino IDE).
//
// Architecture: classic Arduino setup()/loop() with a cooperative scheduler
// (mf_scheduler) that fans tasks out by millis() — no FreeRTOS API used in
// our code, even though the ESP32 Arduino core still relies on FreeRTOS
// internally. Keep handlers short and non-blocking.

#include <Arduino.h>
#include <Wire.h>

#include "mf_config.h"
#include "mf_log.h"
#include "mf_clock.h"
#include "mf_board.h"
#include "mf_resources.h"
#include "mf_scheduler.h"
#include "mf_actuators.h"
#include "mf_sensor_scd41.h"
#include "mf_sensor_mlx90614.h"
#include "mf_sensor_water.h"
#include "mf_service_btn.h"
#include "mf_batch_logger.h"
#include "mf_recipe.h"
#include "mf_fsm.h"
#include "mf_climate.h"

static void task_sensors() {
    mf_scd41_poll();
    mf_mlx90614_poll();
    mf_water_poll();
}

static void task_climate() {
    mf_climate_tick();
}

static void task_trace() {
    mf_log_info("trace", "state=%s stage=%s recipe=%s rh=%.1f%% co2=%.0f t=%.1fC obj=%.1fC water=%d",
                mf_fsm_state_str(mf_fsm_state()),
                mf_recipe_current_stage_id(),
                mf_fsm_selected_recipe_id(),
                mf_scd41_rh_percent(),
                mf_scd41_co2_ppm(),
                mf_scd41_temp_c(),
                mf_mlx90614_object_c(),
                (int)mf_water_present());
}

static void task_recipe() {
    mf_recipe_runtime_tick();
}

static void task_batch_flush() {
    mf_batch_logger_flush();
}

void setup() {
    mf_log_init(115200);
    mf_log_info("boot", "MushFarm %s git=%s", MF_FW_VERSION, MF_GIT_SHORT_SHA);
    mf_board_log_pin_map();
    mf_boot_log_resource_metrics();
#if MF_SENSORS_MOCK
    mf_log_info("boot", "S2.5 sensors=MOCK (synthetic I2C; set MF_SENSORS_MOCK 0 for hardware)");
#else
    mf_log_info("boot", "S2.5 sensors=LIVE (I2C drivers)");
#endif

    mf_actuators_init_safe_off();

    Wire.begin(MF_I2C_SDA_PIN, MF_I2C_SCL_PIN);
    mf_scd41_init();
    mf_mlx90614_init();
    mf_water_init();
    mf_service_btn_init();

    // SD logging is disabled in the skeleton; the RAM ring buffer still runs.
    mf_batch_logger_init(false);

    mf_fsm_resume_restore_from_nvs();
    mf_fsm_boot_done_config_ok();

#if MF_AUTO_DEMO_CYCLE
    if (mf_fsm_state() == MF_STATE_IDLE_READY) {
        mf_fsm_select_recipe("embedded_demo");
        mf_fsm_result_t r = mf_fsm_start_cycle();
        if (r != MF_FSM_OK && r != MF_FSM_NOOP) {
            mf_log_warn("boot", "auto start cycle failed: %d", (int)r);
        }
    }
#endif

    mf_scheduler_add("sensors", MF_TICK_SENSORS_MS, task_sensors);
    mf_scheduler_add("recipe", MF_TICK_RECIPE_MS, task_recipe);
    mf_scheduler_add("climate", MF_TICK_CLIMATE_MS, task_climate);
    mf_scheduler_add("trace", MF_TICK_TRACE_MS, task_trace);
    mf_scheduler_add("batch_flush", MF_TICK_BATCH_FLUSH_MS, task_batch_flush);

    mf_log_info("boot", "ready");
}

void loop() {
    // Service button is polled every iteration so long-press is not missed.
    mf_service_btn_poll();
    mf_scheduler_tick();
    // Yield to the underlying core so Wi-Fi/USB CDC stay alive even though
    // we don't touch FreeRTOS APIs ourselves.
    delay(2);
}
