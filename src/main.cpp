/*
 * Copyright (c) 2021-present LAAS-CNRS
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 2.1 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1
 */

/**
 * @brief  This example demonstrates how to deploy a Buck converter with
 *         voltage mode control on the Twist power shield.
 *
 * @author Clément Foucher <clement.foucher@laas.fr>
 * @author Luiz Villa <luiz.villa@laas.fr>
 * @author Ayoub Farah Hassan <ayoub.farah-hassan@laas.fr>
 */

/*--------------Zephyr---------------------------------------- */
#include <zephyr/console/console.h>

/*--------------OWNTECH APIs---------------------------------- */
#include "SpinAPI.h"
#include "ShieldAPI.h"
#include "TaskAPI.h"
#include "user_data_objects.h"

/*--------------OWNTECH Libraries----------------------------- */
#include "pid.h"

/*--------------SETUP FUNCTIONS DECLARATION------------------- */
/* Setups the hardware and software of the system */
void setup_routine();

/*--------------LOOP FUNCTIONS DECLARATION-------------------- */
/* Code to be executed in the slow communication task */
void loop_communication_task();
/* Code to be executed in the background task */
void loop_application_task();
/* Code to be executed in real time in the critical task */
void loop_critical_task();

/*--------------USER VARIABLES DECLARATIONS------------------- */

/* [us] period of the control task */
static uint32_t control_task_period = 100;
/* [bool] state of the PWM (ctrl task) */
static bool pwm_enable = false;

uint8_t received_serial_char;

/* Measure variables */

// static float32_t V1_low_value;
// static float32_t V2_low_value;
// static float32_t I1_low_value;
// static float32_t I2_low_value;
// static float32_t I_high;
// static float32_t V_high;

// static float32_t temp_1_value;
// static float32_t temp_2_value;

/* Temporary storage fore measured value (ctrl task) */
// static float meas_data;

// float32_t duty_cycle = 0.3;

/* Voltage reference */
// static float32_t voltage_reference = 15;

/* PID coefficients for a 8.6ms step response*/
static float32_t kp = 0.000215;
static float32_t Ti = 7.5175e-5;
static float32_t Td = 0.0;
static float32_t N = 0.0;
static float32_t upper_bound = 1.0F;
static float32_t lower_bound = 0.0F;
static float32_t Ts = control_task_period * 1e-6;
static PidParams pid_params(Ts, kp, Ti, Td, N, lower_bound, upper_bound);
static Pid pid;

/*--------------------------------------------------------------- */

/* LIST OF POSSIBLE MODES FOR THE OWNTECH CONVERTER */
// Mode state
uint8_t mode = MODE_IDL;

void conf_mode_cb(enum thingset_callback_reason reason)
{
    switch (reason) {
        case THINGSET_CALLBACK_PRE_WRITE:
            mode_prev = mode_current;
            break;
        case THINGSET_CALLBACK_POST_WRITE:
            if (mode_current != mode_prev && mode_current < NUM_OF_MODES) {
                // update runtime mode used by application
                mode = mode_current;
                printk("Mode changed: %s (%u)\n", modes[mode].name, (unsigned)mode);
            }
            break;
        default:
            break;
    }
}

// Removed legacy conf_set_leg_on and xSet handlers

// Removed legacy conf_set_ac; superseded by conf_func_cb



// Removed legacy conf_set_leg

// Per-leg subgroup callbacks using a common handler
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
            // wOn, wBuck, wDuty, wRef applied by higher-level logic as needed
            break;
        default:
            break;
    }
}

void conf_leg_cb_0(enum thingset_callback_reason reason) { conf_set_leg_idx(reason, 0); }
void conf_leg_cb_1(enum thingset_callback_reason reason) { conf_set_leg_idx(reason, 1); }
void conf_leg_cb_2(enum thingset_callback_reason reason) { conf_set_leg_idx(reason, 2); }

// ========= Config → Function (unified) callback =========
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
            // Basic range guardrails
            if (func_domain > FUNC_DOMAIN_AC) {
                func_domain = FUNC_DOMAIN_DC;
            }
            if (func_ac_mode > FUNC_AC_GFL) {
                func_ac_mode = FUNC_AC_GF;
            }

            // Domain change handling
            if (func_domain != func_prev.domain) {
                switch ((func_domain_t)func_domain) {
                    case FUNC_DOMAIN_DC:
                        // Optionally disable AC-specific paths
                        break;
                    case FUNC_DOMAIN_AC:
                        // Optionally disable DC-specific paths
                        break;
                }
                printk("Function domain changed: %u -> %u\n",
                       (unsigned)func_prev.domain, (unsigned)func_domain);
            }

            // DC feature toggles
            if (func_dc_vscs_enable != func_prev.dc_vscs) {
                printk("DC VS/CS feature: %s\n", func_dc_vscs_enable ? "EN" : "DIS");
            }
            if (func_dc_droop_enable != func_prev.dc_droop) {
                printk("DC Droop feature: %s\n", func_dc_droop_enable ? "EN" : "DIS");
            }

            // AC mode change
            if (func_ac_mode != func_prev.ac_mode) {
                switch ((func_ac_mode_t)func_ac_mode) {
                    case FUNC_AC_GF:
                        // init grid-forming path as needed
                        break;
                    case FUNC_AC_GFL:
                        // init grid-following path as needed
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


// Removed legacy meas_set_calib

// Unified measurement subgroup callbacks
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


// Removed legacy AC xSet helpers



// void idling_power()
// {
//     mode = IDLEMODE;
// }

// void set_voltage(void)
// {
//     voltage_reference = voltage_setpoint;
// }
/*--------------SETUP FUNCTIONS------------------------------- */

/**
 * This is the setup routine.
 * Here the setup :
 *  - Initializes the power shield in Buck mode
 *  - Initializes the power shield sensors
 *  - Initializes the PID controller
 *  - Spawns three tasks.
 */
void setup_routine()
{
    /* Buck voltage mode */
    shield.power.initBuck(ALL);

    shield.sensors.enableDefaultTwistSensors();

    // Load stored calibration (gain/offset) from NVM into runtime map
    for (int i = 0; i < NUM_OF_MEAS; i++) {
        float32_t g = shield.sensors.retrieveStoredParameterValue(system_sensors[i].channel_reference, gain);
        float32_t o = shield.sensors.retrieveStoredParameterValue(system_sensors[i].channel_reference, offset);
        system_sensors[i].gain = g;
        system_sensors[i].offset = o;
    }

    pid.init(pid_params);

    /* Then declare tasks */
    uint32_t app_task_number = task.createBackground(loop_application_task);
    uint32_t com_task_number = task.createBackground(loop_communication_task);
    task.createCritical(loop_critical_task, 100);

    /* Finally, start tasks */
    task.startBackground(app_task_number);
    task.startBackground(com_task_number);
    task.startCritical();
}

/*--------------LOOP FUNCTIONS-------------------------------- */

/**
 * This tasks implements a minimalistic USB serial interface to control
 * the buck converter.
 */
void loop_communication_task()
{
    received_serial_char = console_getchar();
    switch (received_serial_char)
    {
    case 'h':
        /*----------SERIAL INTERFACE MENU----------------------- */
        printk(" ________________________________________ \n"
               "|     ---- MENU buck voltage mode ----   |\n"
               "|     press i : idle mode                |\n"
               "|     press p : power mode               |\n"
               "|     press u : voltage reference UP     |\n"
               "|     press d : voltage reference DOWN   |\n"
               "|________________________________________|\n\n");
        /*------------------------------------------------------ */
        break;
    case 'i':
        printk("idle mode\n");
        mode = MODE_IDL;
        break;
    case 'p':
        printk("power mode\n");
        mode = MODE_PWR;
        break;
    case 'u':
        voltage_reference += 0.5;
        break;
    case 'd':
        voltage_reference -= 0.5;
        break;
    default:
        break;
    }
}

/**
 * This is the code loop of the background task
 * This task mostly logs back measurements to the USB serial interface.
 */
void loop_application_task()
{
    static int print_counter = 0;
    if (mode == MODE_IDL)
    {
        spin.led.turnOff();
    }
    else if (mode == MODE_PWR)
    {
        spin.led.turnOn();

        shield.sensors.triggerTwistTempMeas(TEMP_SENSOR_1);
        shield.sensors.triggerTwistTempMeas(TEMP_SENSOR_2);

        meas_data = shield.sensors.getLatestValue(TEMP_SENSOR_1);
        if (meas_data != NO_VALUE) temp_1_value = meas_data;

        meas_data = shield.sensors.getLatestValue(TEMP_SENSOR_2);
        if (meas_data != NO_VALUE) temp_2_value = meas_data;


        printk("%.3f:", (double)I1_low_value);
        printk("%.3f:", (double)V1_low_value);
        printk("%.3f:", (double)voltage_reference);
        printk("%.3f:", (double)I2_low_value);
        printk("%.3f:", (double)V2_low_value);
        printk("%.3f:", (double)voltage_reference);
        printk("%.3f:", (double)I_high_value);
        printk("%.3f:", (double)V_high_value);
        printk("%.3f:", (double)temp_1_value);
        // printk("%.3f:", (double)power_legs[received_leg_number].wLegON);
        printk("\n");


    }
    task.suspendBackgroundMs(100);
}

/**
 * This is the code loop of the critical task
 * This task runs at 10kHz.
 *  - It retrieves sensors values
 *  - It runs the PID controller
 *  - It update the PWM signals
 */
void loop_critical_task()
{
    meas_data = shield.sensors.getLatestValue(I1_LOW);
    if (meas_data != NO_VALUE) I1_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(V1_LOW);
    if (meas_data != NO_VALUE) V1_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(V2_LOW);
    if (meas_data != NO_VALUE) V2_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(I2_LOW);
    if (meas_data != NO_VALUE) I2_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(I_HIGH);
    if (meas_data != NO_VALUE) I_high_value = meas_data;

    meas_data = shield.sensors.getLatestValue(V_HIGH);
    if (meas_data != NO_VALUE) V_high_value = meas_data;


    if (mode == MODE_IDL)
    {
        if (pwm_enable == true)
        {
            shield.power.stop(ALL);
        }
        pwm_enable = false;
    }
    else if (mode == MODE_PWR)
    {
        duty_cycle = pid.calculateWithReturn(voltage_reference, V1_low_value);
        shield.power.setDutyCycle(ALL,duty_cycle);

        /* Set POWER ON */
        if (!pwm_enable)
        {
            pwm_enable = true;
            shield.power.start(ALL);
        }
    }

}

/**
 * This is the main function of this example
 * This function is generic and does not need editing.
 */
int main(void)
{
    setup_routine();

    return 0;
}
