idf_component_register(
    SRCS
        "mf_system.c"
        "mf_batch_logger.c"
        "mf_log_ring.c"
        "mf_time_sync.c"
        "mf_sd_log.c"
        "mf_service_btn.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer vfs fatfs sdmmc esp_netif mf_control lwip
)
