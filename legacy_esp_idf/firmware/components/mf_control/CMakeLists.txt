idf_component_register(
    SRCS
        "mf_fsm.c"
        "mf_recipe.c"
        "mf_climate.c"
        "mf_arbiter.c"
    INCLUDE_DIRS "include"
    REQUIRES mf_drivers json esp_timer nvs_flash
)
