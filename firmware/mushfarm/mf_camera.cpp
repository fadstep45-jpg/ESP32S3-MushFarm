#include "mf_camera.h"
#include "mf_config.h"
#include "mf_log.h"
#include "mf_fsm.h"

#if MF_CAMERA_ENABLE
static const char *s_status = "stub";
#else
static const char *s_status = "disabled";
#endif

bool mf_camera_init() {
#if MF_CAMERA_ENABLE
    // No real driver yet — fail closed. Set sticky WARN_CAMERA_FAIL per
    // state-machine.md "Camera Behavior per State": camera failures
    // NEVER trigger FSM transitions, only a sticky observability flag.
    mf_log_warn("camera", "MF_CAMERA_ENABLE=1 but driver not wired yet; status=stub");
    s_status = "stub_unimplemented";
    mf_fsm_set_warn(MF_WARN_CAMERA_FAIL);
#else
    // Deliberately disabled by config (default): no warn, not a fault.
    mf_log_info("camera", "camera=disabled (compile-time)");
    s_status = "disabled";
#endif
    return true;
}

void mf_camera_poll() {
}

const char *mf_camera_status_str() {
    return s_status;
}
