#include "mf_pid.h"

void mf_pid_init(mf_pid_t *pid, float kp, float ki, float kd, float out_min, float out_max) {
    if (!pid) return;
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->out_min = out_min;
    pid->out_max = out_max;
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->integrator_frozen = false;
}

void mf_pid_reset(mf_pid_t *pid) {
    if (!pid) return;
    pid->integral = 0.0f;
    pid->last_error = 0.0f;
    pid->integrator_frozen = false;
}

float mf_pid_step(mf_pid_t *pid, float error, float dt_sec) {
    if (!pid || dt_sec <= 0.0f) {
        return 0.0f;
    }
    float p = pid->kp * error;
    if (!pid->integrator_frozen) {
        pid->integral += pid->ki * error * dt_sec;
    }
    float d = pid->kd * (error - pid->last_error) / dt_sec;
    pid->last_error = error;
    float out = p + pid->integral + d;
    if (out > pid->out_max) {
        out = pid->out_max;
        pid->integrator_frozen = true;
    } else if (out < pid->out_min) {
        out = pid->out_min;
        pid->integrator_frozen = true;
    } else {
        pid->integrator_frozen = false;
    }
    return out;
}

float mf_pid_slew(float prev_out, float new_out, float max_delta_per_sec, float dt_sec) {
    if (dt_sec <= 0.0f || max_delta_per_sec <= 0.0f) {
        return new_out;
    }
    float max_step = max_delta_per_sec * dt_sec;
    float delta = new_out - prev_out;
    if (delta > max_step) {
        return prev_out + max_step;
    }
    if (delta < -max_step) {
        return prev_out - max_step;
    }
    return new_out;
}
