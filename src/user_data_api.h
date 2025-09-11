#ifndef USER_DATA_API_H
#define USER_DATA_API_H

#include <stdint.h>
#include <stdbool.h>

#include "ShieldAPI.h" // for sensor_t

/* Mirror selected types and extern variables from user_data_objects.h
 * to allow other translation units to use the shared runtime storage
 * without including the Thingset registry macros. */

typedef struct {
    const char *name;
    uint8_t value;
} ModeDef;

extern uint8_t mode_current;
extern uint8_t mode_prev;
extern const ModeDef modes[];
enum { MODE_IDL = 0, MODE_PWR = 1, NUM_OF_MODES = 2 };

/* Measurements */
extern float32_t V1_low_value;
extern float32_t V2_low_value;
extern float32_t V_high_value;
extern float32_t I1_low_value;
extern float32_t I2_low_value;
extern float32_t I_high_value;

#ifdef CONFIG_SHIELD_OWNVERTER
extern float32_t V3_low_value;
extern float32_t I3_low_value;
extern float32_t temp_value;
#else
extern float32_t temp_1_value;
extern float32_t temp_2_value;
#endif

extern float32_t duty_cycle;
extern float32_t voltage_reference;
extern float32_t voltage_setpoint;
extern float32_t meas_data;

// Debug scope dump trigger storage
extern bool dbg_scope_dump;
extern bool dbg_scope_trig;

// Inverter control references (ThingSet-controlled)
extern float32_t ctrl_vd_ref;
extern float32_t ctrl_vq_ref;
extern float32_t ctrl_omega_ref;

typedef struct {
    const char *name;
    float32_t *address;
    sensor_t    channel_reference;
    float32_t   gain;
    float32_t   offset;
} SystemSensors;

extern SystemSensors system_sensors[];

#ifndef POWER_NUM_LEGS
    #ifdef CONFIG_SHIELD_OWNVERTER
        #define POWER_NUM_LEGS  3
    #else
        #define POWER_NUM_LEGS  2
    #endif
#endif

typedef struct {
    int8_t  wVar;
    float32_t   wRef;
    float32_t *tracking_var;
    const char *tracking_name;
    bool    wLegON;
    bool    wCapa;
    bool    wDriver;
    bool    wBuck;
    float32_t   wDuty;
    float32_t   wPhase_deg;
    uint16_t wDead_rise_ns;
    uint16_t wDead_fall_ns;
    float32_t   wFreq_Hz;
} power_leg_t;

extern power_leg_t power_legs[POWER_NUM_LEGS];

enum { NUM_OF_MEAS = 
#ifdef CONFIG_SHIELD_OWNVERTER
    8
#else
    6
#endif
};

// Minimal function selection flags
typedef enum { FUNC_DOMAIN_DC = 0, FUNC_DOMAIN_AC = 1 } func_domain_t;
typedef enum { FUNC_AC_GF = 0, FUNC_AC_GFL = 1 } func_ac_mode_t;

extern uint8_t func_domain;
extern bool    func_dc_vscs_enable;
extern bool    func_dc_droop_enable;
extern uint8_t func_ac_mode;

/* Application hooks used by ThingSet callbacks */
void app_apply_ac_mode(uint8_t new_ac_mode);
// Debug hook used by ThingSet to dump ScopeMimicry data
void app_dump_scope(void);
// Set ScopeMimicry manual trigger (ThingSet writes)
void app_set_scope_trigger(bool trig);
// Apply Vdq and omega references from ThingSet
void app_apply_ctrl_refs(float32_t vd, float32_t vq, float32_t omega);

#endif // USER_DATA_API_H
