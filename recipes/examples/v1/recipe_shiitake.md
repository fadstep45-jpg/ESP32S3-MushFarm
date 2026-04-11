recipe_meta:
  recipe_id: "shiitake_pro_v5"
  recipe_name: "Шиитаке (Lentinula edodes) - Приток + MOSFET"
  species: "Lentinula edodes"
  version: 5
  author: "Myco IoT Core Architect"
  created_at: "2026-04-11"
  description: "Особенности: очень долгая инкубация и мягкий свет на плодоношении."
  notes: "Глобальная авария по CO2 отключена (null). Учет аппаратных лимитов SCD41."

execution_policy:
  start_mode: "manual_select_then_start"
  resume_after_reboot: true
  allow_hot_patch_stage_params: true
  data_logging_profile: "high_frequency_event_driven"

hardware_profile:
  sensor_set_required:
    - scd41
    - mlx90614
    - xkc_y25
  actuator_set_required:
    - inlet_fan
    - humidifier
    - grow_light
  minimum_sensor_set_for_start:
    - scd41
    - xkc_y25

global_targets:
  co2_control_enabled: true
  humidity_control_enabled: true
  temp_control_enabled: true
  light_control_enabled: true

safety_profile:
  hard_limits:
    co2_crit_ppm: null
    rh_min_crit_percent: 50
    rh_max_crit_percent: 99
    temp_min_crit_c: 5
    temp_max_crit_c: 35
  harvest_first_overrides:
    humidifier_blind_pulse_sec: 15
    humidifier_blind_pause_sec: 180
    blind_cycle_max_retries: 3
    prophylactic_ventilation_interval_hours: 4
    prophylactic_ventilation_duration_min: 2

stages:
  - stage_id: "S1_colonization"
    auto_transition_after_days: 60
    transition_rule: "manual_after_browning"
    setpoints:
      rh_target_percent: 80
      co2_target_ppm: 5000
      air_temp_target_c: 24
      substrate_temp_target_c: 25
      light_hours_per_day: 0
    control:
      rh_hysteresis_percent: 5
      co2_hysteresis_ppm: 500
      max_substrate_air_delta_c: 8.0
      inlet_fan_min_duty_percent: 0
      inlet_fan_max_duty_percent: 15
      humidifier_max_duty_percent: 50
      grow_light_duty_percent: 0
      control_tick_sec: 2
    arbitration:
      allow_parallel_inlet_and_humidifier: false
      ignore_high_co2_limit: true
      co2_emergency_purge_enabled: false
      purge_duration_sec: 0
    alerts:
      stage_warn_repeat_min: 120
      stage_error_repeat_min: 30

  - stage_id: "S2_pinning"
    auto_transition_after_days: 4
    transition_rule: "auto_after_cold_shock"
    setpoints:
      rh_target_percent: 95
      co2_target_ppm: 900
      air_temp_target_c: 12
      substrate_temp_target_c: 14
      light_hours_per_day: 10
    control:
      rh_hysteresis_percent: 2
      co2_hysteresis_ppm: 150
      max_substrate_air_delta_c: 4.0
      inlet_fan_min_duty_percent: 20
      inlet_fan_max_duty_percent: 100
      humidifier_max_duty_percent: 100
      grow_light_duty_percent: 100
      control_tick_sec: 2
    arbitration:
      allow_parallel_inlet_and_humidifier: true
      ignore_high_co2_limit: false
      co2_emergency_purge_enabled: true
      purge_duration_sec: 60
    alerts:
      stage_warn_repeat_min: 30
      stage_error_repeat_min: 10

  - stage_id: "S3_fruiting"
    auto_transition_after_days: 30
    transition_rule: "manual_harvest_stop"
    setpoints:
      rh_target_percent: 70
      co2_target_ppm: 900
      air_temp_target_c: 18
      substrate_temp_target_c: 19
      light_hours_per_day: 10
    control:
      rh_hysteresis_percent: 5
      co2_hysteresis_ppm: 200
      max_substrate_air_delta_c: 2.0
      inlet_fan_min_duty_percent: 15
      inlet_fan_max_duty_percent: 80
      humidifier_max_duty_percent: 50
      grow_light_duty_percent: 60
      control_tick_sec: 2
    arbitration:
      allow_parallel_inlet_and_humidifier: true
      ignore_high_co2_limit: false
      co2_emergency_purge_enabled: true
      purge_duration_sec: 60
    alerts:
      stage_warn_repeat_min: 30
      stage_error_repeat_min: 10

acceptance_criteria:
  - "Мягкое диммирование света на стадии плодоношения для избежания ожогов шляпок"
  - "Ограничение по критическому уровню CO2 (Авария) снято для защиты от конфликтов"
  - "Целевой уровень CO2 установлен в 5000ppm для штатной работы ПИД-регулятора на инкубации"