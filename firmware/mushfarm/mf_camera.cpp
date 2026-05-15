#include "mf_camera.h"
#include "mf_config.h"
#include "mf_log.h"

#if MF_CAMERA_ENABLE
static const char *s_status = "stub";
#else
static const char *s_status = "disabled";
#endif

bool mf_camera_init() {
#if MF_CAMERA_ENABLE
    mf_log_warn("camera", "MF_CAMERA_ENABLE=1 but driver not wired yet; status=stub");
    s_status = "stub";
#else
    mf_log_info("camera", "camera=disabled (stub)");
    s_status = "disabled";
#endif
    return true;
}

void mf_camera_poll() {
}

const char *mf_camera_status_str() {
    return s_status;
}
