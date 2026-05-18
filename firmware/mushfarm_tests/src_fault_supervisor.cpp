// Unit tests do not link mf_mqtt; disable alert publish in fault_supervisor.
#define MF_MQTT_ENABLE 0
#include "../mushfarm/mf_fault_supervisor.cpp"
