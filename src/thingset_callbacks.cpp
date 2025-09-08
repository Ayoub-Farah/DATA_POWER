/* Thingset callbacks extracted from main.cpp to declutter the application loop */

#include <zephyr/sys/printk.h>
#include <thingset.h>

#include "SpinAPI.h"
#include "ShieldAPI.h"
#include "user_data_api.h"

// runtime app mode lives in main.cpp
extern uint8_t mode;

/* ===== Mode callback ===== */
void conf_mode_cb(enum thingset_callback_reason reason)
{
    switch (reason) {
        case THINGSET_CALLBACK_PRE_WRITE:
            mode_prev = mode_current;
            break;
        case THINGSET_CALLBACK_POST_WRITE:
            if (mode_current != mode_prev && mode_current < NUM_OF_MODES) {
                mode = mode_current;
                printk("Mode changed: %s (%u)\n", modes[mode].name, (unsigned)mode);
            }
            break;
        default:
            break;
    }
}

/* ===== Per-leg subgroup callbacks (shared handler) ===== */
typedef struct {
    bool on, capa, driver, buck;
    float32_t duty, phase_deg, freq_hz, ref;
    uint16_t dtrise, dtfall;
    int8_t var;
} leg_shadow_t;

static leg_shadow_t leg_prev[POWER_NUM_LEGS];

static void conf_set_leg_idx(enum thingset_callback_reason reason, uint8_t i)
{
    switch (reason) {
        case THINGSET_CALLBACK_PRE_WRITE:
            leg_prev[i].on       = power_legs[i].wLegON;
            leg_prev[i].capa     = power_legs[i].wCapa;
            leg_prev[i].driver   = power_legs[i].wDriver;
            leg_prev[i].buck     = power_legs[i].wBuck;
            leg_prev[i].duty     = power_legs[i].wDuty;
            leg_prev[i].phase_deg= power_legs[i].wPhase_deg;
            leg_prev[i].dtrise   = power_legs[i].wDead_rise_ns;
            leg_prev[i].dtfall   = power_legs[i].wDead_fall_ns;
            leg_prev[i].freq_hz  = power_legs[i].wFreq_Hz;
            leg_prev[i].var      = power_legs[i].wVar;
            leg_prev[i].ref      = power_legs[i].wRef;
            break;

        case THINGSET_CALLBACK_POST_WRITE:
            if (power_legs[i].wCapa != leg_prev[i].capa) {
                if (power_legs[i].wCapa)
                    shield.power.connectCapacitor((leg_t)i);
                else
                    shield.power.disconnectCapacitor((leg_t)i);
            }
            if (power_legs[i].wDriver != leg_prev[i].driver) {
                if (power_legs[i].wDriver)
                    shield.power.connectDriver((leg_t)i);
                else
                    shield.power.disconnectDriver((leg_t)i);
            }
            if (power_legs[i].wPhase_deg != leg_prev[i].phase_deg) {
                shield.power.setPhaseShift((leg_t)i, (int16_t)power_legs[i].wPhase_deg);
            }
            if (power_legs[i].wDead_rise_ns != leg_prev[i].dtrise ||
                power_legs[i].wDead_fall_ns != leg_prev[i].dtfall) {
                shield.power.setDeadTime((leg_t)i, power_legs[i].wDead_rise_ns, power_legs[i].wDead_fall_ns);
            }
            if (power_legs[i].wFreq_Hz != leg_prev[i].freq_hz) {
                spin.pwm.setFrequency((uint32_t)power_legs[i].wFreq_Hz);
            }
            if (power_legs[i].wVar != leg_prev[i].var) {
                if (power_legs[i].wVar >= 0 && power_legs[i].wVar < NUM_OF_MEAS) {
                    power_legs[i].tracking_var  = system_sensors[(int)power_legs[i].wVar].address;
                    power_legs[i].tracking_name = system_sensors[(int)power_legs[i].wVar].name;
                }
            }
            break;
        default:
            break;
    }
}

void conf_leg_cb_0(enum thingset_callback_reason reason) { conf_set_leg_idx(reason, 0); }
void conf_leg_cb_1(enum thingset_callback_reason reason) { conf_set_leg_idx(reason, 1); }
void conf_leg_cb_2(enum thingset_callback_reason reason) { conf_set_leg_idx(reason, 2); }

/* ===== Function (unified) callback ===== */
typedef struct {
    uint8_t domain;
    bool    dc_vscs;
    bool    dc_droop;
    uint8_t ac_mode;
} func_shadow_t;

static func_shadow_t func_prev;

void conf_func_cb(enum thingset_callback_reason reason)
{
    switch (reason) {
        case THINGSET_CALLBACK_PRE_WRITE:
            func_prev.domain  = func_domain;
            func_prev.dc_vscs = func_dc_vscs_enable;
            func_prev.dc_droop= func_dc_droop_enable;
            func_prev.ac_mode = func_ac_mode;
            break;

        case THINGSET_CALLBACK_POST_WRITE:
            if (func_domain > FUNC_DOMAIN_AC) {
                func_domain = FUNC_DOMAIN_DC;
            }
            if (func_ac_mode > FUNC_AC_GFL) {
                func_ac_mode = FUNC_AC_GF;
            }

            if (func_domain != func_prev.domain) {
                switch ((func_domain_t)func_domain) {
                    case FUNC_DOMAIN_DC:
                        break;
                    case FUNC_DOMAIN_AC:
                        break;
                }
                printk("Function domain changed: %u -> %u\n",
                       (unsigned)func_prev.domain, (unsigned)func_domain);
            }

            if (func_dc_vscs_enable != func_prev.dc_vscs) {
                printk("DC VS/CS feature: %s\n", func_dc_vscs_enable ? "EN" : "DIS");
            }
            if (func_dc_droop_enable != func_prev.dc_droop) {
                printk("DC Droop feature: %s\n", func_dc_droop_enable ? "EN" : "DIS");
            }

            if (func_ac_mode != func_prev.ac_mode) {
                switch ((func_ac_mode_t)func_ac_mode) {
                    case FUNC_AC_GF:
                        break;
                    case FUNC_AC_GFL:
                        break;
                }
                printk("AC mode changed: %u -> %u\n",
                       (unsigned)func_prev.ac_mode, (unsigned)func_ac_mode);
            }
            break;
        default:
            break;
    }
}

/* ===== Measurement subgroup callbacks ===== */
static void meas_cb_idx(enum thingset_callback_reason reason, uint8_t idx)
{
    switch (reason) {
        case THINGSET_CALLBACK_POST_WRITE:
            shield.sensors.setConversionParametersLinear(
                system_sensors[idx].channel_reference,
                system_sensors[idx].gain,
                system_sensors[idx].offset);
            break;
        default:
            break;
    }
}

void meas_cb_0(enum thingset_callback_reason reason) { meas_cb_idx(reason, 0); }
void meas_cb_1(enum thingset_callback_reason reason) { meas_cb_idx(reason, 1); }
void meas_cb_2(enum thingset_callback_reason reason) { meas_cb_idx(reason, 2); }
void meas_cb_3(enum thingset_callback_reason reason) { meas_cb_idx(reason, 3); }
void meas_cb_4(enum thingset_callback_reason reason) { meas_cb_idx(reason, 4); }
void meas_cb_5(enum thingset_callback_reason reason) { meas_cb_idx(reason, 5); }
#ifdef CONFIG_SHIELD_OWNVERTER
void meas_cb_6(enum thingset_callback_reason reason) { meas_cb_idx(reason, 6); }
void meas_cb_7(enum thingset_callback_reason reason) { meas_cb_idx(reason, 7); }
#endif
