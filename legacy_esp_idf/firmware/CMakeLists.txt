# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16)

set(SUPPORTED_TARGETS esp32s3)
set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Application version (embedded in esp_app_desc)
set(PROJECT_VER "0.1.0")
project(mushfarm)
