#pragma once

// Shared ACK/error codes for HTTP and MQTT (ap-contract / mqtt-contract).

#define MF_API_ACK_OK                 "ACK_OK"
#define MF_API_ACK_NOOP               "ACK_NOOP"
#define MF_API_ERR_STATE              "ERR_STATE"
#define MF_API_ERR_LATCHED            "ERR_LATCHED"
#define MF_API_ERR_SCHEMA_INVALID     "ERR_SCHEMA_INVALID"
#define MF_API_ERR_PAYLOAD_TOO_LARGE  "ERR_PAYLOAD_TOO_LARGE"
#define MF_API_ERR_NOT_IMPLEMENTED    "ERR_NOT_IMPLEMENTED"
#define MF_API_ERR_SAFETY_LIMIT       "ERR_SAFETY_LIMIT"
#define MF_API_ERR_RECIPE_NOT_FOUND   "ERR_RECIPE_NOT_FOUND"
#define MF_API_ERR_RECIPE_NOT_SELECTED "ERR_RECIPE_NOT_SELECTED"
#define MF_API_ERR_NOT_FOUND          "ERR_NOT_FOUND"
