idf_component_register(
    SRCS
        "mf_board.c"
        "mf_i2c_bus.c"
        "mf_sensor_scd41.c"
        "mf_sensor_mlx90614.c"
        "mf_sensor_water.c"
        "mf_actuator_pwm.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_driver_ledc esp_driver_i2c esp_timer
)
