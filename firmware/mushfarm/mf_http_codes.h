#pragma once

#include "mf_api_codes.h"

// Backward-compatible aliases for HTTP handlers.
#define MF_HTTP_ACK_OK                 MF_API_ACK_OK
#define MF_HTTP_ERR_STATE              MF_API_ERR_STATE
#define MF_HTTP_ERR_LATCHED            MF_API_ERR_LATCHED
#define MF_HTTP_ERR_SCHEMA_INVALID     MF_API_ERR_SCHEMA_INVALID
#define MF_HTTP_ERR_PAYLOAD_TOO_LARGE  MF_API_ERR_PAYLOAD_TOO_LARGE
#define MF_HTTP_ERR_NOT_IMPLEMENTED    MF_API_ERR_NOT_IMPLEMENTED
#define MF_HTTP_ERR_SAFETY_LIMIT       MF_API_ERR_SAFETY_LIMIT
