idf_component_register(
    SRCS
        "mf_connectivity.c"
        "mf_wifi_ap.c"
        "mf_http_api.c"
        "mf_mqtt_app.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp_event esp_http_server mqtt mf_control json
)
