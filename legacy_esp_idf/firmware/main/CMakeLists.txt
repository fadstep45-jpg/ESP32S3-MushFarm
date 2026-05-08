# SPDX-License-Identifier: MIT
set(_mf_repo_root "${CMAKE_SOURCE_DIR}/..")
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${_mf_repo_root}"
    OUTPUT_VARIABLE MF_GIT_SHORT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT MF_GIT_SHORT_SHA OR MF_GIT_SHORT_SHA STREQUAL "")
    set(MF_GIT_SHORT_SHA "unknown")
endif()

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/fw_build_info.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fw_build_info.h"
    @ONLY
)

set(srcs
    "app_main.c"
    "mf_app.c"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "." "${CMAKE_CURRENT_BINARY_DIR}"
    REQUIRES mf_drivers mf_control mf_connectivity mf_system
        esp_app_format nvs_flash esp_netif
)
