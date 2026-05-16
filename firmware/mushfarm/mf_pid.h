#pragma once

#include <stdbool.h>

typedef struct mf_pid_t {
    float kp;
    float ki;
    float kd;
    float out_min;
    float out_max;
    float integral;
    float last_error;
    bool integrator_frozen;
} mf_pid_t;

void mf_pid_init(mf_pid_t *pid, float kp, float ki, float kd, float out_min, float out_max);
void mf_pid_reset(mf_pid_t *pid);
float mf_pid_step(mf_pid_t *pid, float error, float dt_sec);
float mf_pid_slew(float prev_out, float new_out, float max_delta_per_sec, float dt_sec);
