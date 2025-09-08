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
enum serial_interface_menu_mode
{
    IDLEMODE = 0,
    POWERMODE = 1
};

uint8_t idle_mode = IDLEMODE;
uint8_t power_mode = POWERMODE;
uint8_t mode = IDLEMODE;

void conf_set_mode()
{
    if(local_mode<=POWERMODE)
    {
        mode = local_mode;
    }
}

void conf_set_leg_on()
{
    power_legs[received_leg_number].wLegON = leg_set_value;   
}

void conf_set_leg(void)
{
    if (received_leg_number >= POWER_NUM_LEGS ||
        received_param_number >= NUM_OF_PARAMS) {
        return; // out of bounds
    }

    switch (received_param_number) {
        case PARAM_LEG_ON:
            if (received_value>0.0)
                power_legs[received_leg_number].wLegON = true;   // enable leg
            else
                power_legs[received_leg_number].wLegON = false;  // disable leg
            break;

        case PARAM_CAPA:
            if (received_value>0.0)
            {
                power_legs[received_leg_number].wCapa = true;   // enable leg
                shield.power.connectCapacitor((leg_t)received_leg_number);
            }
            else
            {        
                power_legs[received_leg_number].wCapa = false;   // enable leg
                shield.power.disconnectCapacitor((leg_t)received_leg_number);
            }
                break;

        case PARAM_DRIVER:
            if (received_value>0.0)
            {
                power_legs[received_leg_number].wDriver = true;   // enable leg
                shield.power.connectDriver((leg_t)received_leg_number);
            }
            else
            {
                power_legs[received_leg_number].wDriver = false;   // enable leg
                shield.power.disconnectDriver((leg_t)received_leg_number);
            }
            break;

        case PARAM_BUCK:
            power_legs[received_leg_number].wBuck = (bool)received_value;
            // TODO: confirm if there is a spin/shield call for buck mode
            break;


        case PARAM_DUTY:
            power_legs[received_leg_number].wDuty = received_value;
            // Legacy handler uses shield/spin call internally → we can inline it if needed
            break;

        case PARAM_PHASE:
            power_legs[received_leg_number].wPhase_deg = (int16_t)received_value;
            shield.power.setPhaseShift((leg_t)received_leg_number,
                                       (int16_t)received_value);
            break;

        case PARAM_DTRISE:
            power_legs[received_leg_number].wDead_rise_ns = (int16_t)received_value;
            shield.power.setDeadTime((leg_t)received_leg_number,
                                     (uint16_t)received_value,
                                     power_legs[received_leg_number].wDead_fall_ns);
            break;

        case PARAM_DTFALL:
            power_legs[received_leg_number].wDead_fall_ns = (int16_t)received_value;
            shield.power.setDeadTime((leg_t)received_leg_number,
                                     power_legs[received_leg_number].wDead_rise_ns,
                                     (uint16_t)received_value);
            break;

        case PARAM_FREQ:
            power_legs[received_leg_number].wFreq_Hz = (uint32_t)received_value;
            spin.pwm.setFrequency((uint32_t)received_value);
            break;

        case PARAM_VAR:
            // Store which measurement this leg should track
            power_legs[received_leg_number].wVar = (int8_t)received_value;

            if (received_value < NUM_OF_MEAS) {
                power_legs[received_leg_number].tracking_var  = system_sensors[(int)received_value].address;
                power_legs[received_leg_number].tracking_name = system_sensors[(int)received_value].name;
            } else {
                /* Does nothing */
            }
            break;

        case PARAM_REF:
            // Set the reference value directly
            power_legs[received_leg_number].wRef = (float32_t)received_value;
            break;

    }
}


void meas_set_calib(void)
{
    if (received_meas_number >= NUM_OF_MEAS) {
        return; // out of bounds
    }
    switch (received_meas_number) {
        case MEAS_V1:
            shield.sensors.setConversionParametersLinear(V1_LOW, 
                                                         received_meas_gain, 
                                                         received_meas_offset);
            break;

        case MEAS_V2:
            shield.sensors.setConversionParametersLinear(V2_LOW, 
                                                         received_meas_gain, 
                                                         received_meas_offset);
            break;

        case MEAS_VH:
            shield.sensors.setConversionParametersLinear(V_HIGH, 
                                                         received_meas_gain, 
                                                         received_meas_offset);
            break;

        case MEAS_I1:
            shield.sensors.setConversionParametersLinear(I1_LOW, 
                                                         received_meas_gain, 
                                                         received_meas_offset);
            break;

        case MEAS_I2:
            shield.sensors.setConversionParametersLinear(I2_LOW, 
                                                         received_meas_gain, 
                                                         received_meas_offset);
            break;

        case MEAS_IH:
            shield.sensors.setConversionParametersLinear(I_HIGH, 
                                                         received_meas_gain, 
                                                         received_meas_offset);
            break;

#ifdef CONFIG_SHIELD_OWNVERTER
        case MEAS_V3:
            shield.sensors.setConversionParametersLinear(V3_LOW, received_meas_gain, received_meas_offset);
            break;

        case MEAS_I3:
            shield.sensors.setConversionParametersLinear(I3_LOW, received_meas_gain, received_meas_offset);
            break;
#endif

        default:
            // no-op
            break;
    }
}


void conf_set_ac_mode(void) {
    if (received_ac_mode >= NUM_OF_AC_MODES) return;

    switch ((ac_mode_t)received_ac_mode) {
        case AC_MODE_GRID_FORMING:
            // setup grid-forming inverter
            break;
        case AC_MODE_GRID_FOLLOWING:
            // setup grid-following inverter
            break;
    }
}

void conf_set_ac_param(void) {
    if (received_ac_param >= NUM_OF_AC_PARAMS) return;

    switch ((ac_param_t)received_ac_param) {
        case AC_PARAM_P:
            // control active power P
            break;
        case AC_PARAM_Q:
            // control reactive power Q
            break;
    }
}



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
        mode = IDLEMODE;
        break;
    case 'p':
        printk("power mode\n");
        mode = POWERMODE;
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
    if (mode == IDLEMODE)
    {
        spin.led.turnOff();
    }
    else if (mode == POWERMODE)
    {
        spin.led.turnOn();

        shield.sensors.triggerTwistTempMeas(TEMP_SENSOR_1);
        shield.sensors.triggerTwistTempMeas(TEMP_SENSOR_2);

        meas_data = shield.sensors.getLatestValue(TEMP_SENSOR_1);
        if (meas_data != NO_VALUE) temp_1_value = meas_data;

        meas_data = shield.sensors.getLatestValue(TEMP_SENSOR_2);
        if (meas_data != NO_VALUE) temp_2_value = meas_data;


        // printk("%.3f:", (double)I1_low_value);
        // printk("%.3f:", (double)V1_low_value);
        // printk("%.3f:", (double)voltage_reference);
        // printk("%.3f:", (double)I2_low_value);
        // printk("%.3f:", (double)V2_low_value);
        // printk("%.3f:", (double)voltage_reference);
        // printk("%.3f:", (double)I_high_value);
        // printk("%.3f:", (double)V_high_value);
        // printk("%.3f:", (double)temp_1_value);
        // printk("%.3f:", (double)power_legs[received_leg_number].wLegON);
        // printk("\n");

        // printk("Leg %d: ", received_leg_number);

        // printk("ON=%d:",    (int)power_legs[received_leg_number].wLegON);
        // printk("CAPA=%d:",  (int)power_legs[received_leg_number].wCapa);
        // printk("DRV=%d:",   (int)power_legs[received_leg_number].wDriver);
        // printk("BUCK=%d:",  (int)power_legs[received_leg_number].wBuck);
        // printk("BOOST=%d:", (int)power_legs[received_leg_number].wBoost);

        // printk("DUTY=%.3f:",   (double)power_legs[received_leg_number].wDuty);
        // printk("PHASE=%.3f:",  (double)power_legs[received_leg_number].wPhase_deg);
        // printk("DTRISE=%u:",   (unsigned int)power_legs[received_leg_number].wDead_rise_ns);
        // printk("DTFALL=%u:",   (unsigned int)power_legs[received_leg_number].wDead_fall_ns);
        // printk("FREQ=%.3f",    (double)power_legs[received_leg_number].wFreq_Hz);

        // float32_t local_gain, local_offset;

        // /* V1 */
        // local_gain   = shield.sensors.retrieveStoredParameterValue(V1_LOW, gain);
        // local_offset = shield.sensors.retrieveStoredParameterValue(V1_LOW, offset);
        // printk("V1: GAIN=%.4f OFFSET=%.4f RAW=%.3f\n",
        //     (double)local_gain, (double)local_offset, (double)V1_low_value);

        // /* V2 */
        // local_gain   = shield.sensors.retrieveStoredParameterValue(V2_LOW, gain);
        // local_offset = shield.sensors.retrieveStoredParameterValue(V2_LOW, offset);
        // printk("V2: GAIN=%.4f OFFSET=%.4f RAW=%.3f\n",
        //     (double)local_gain, (double)local_offset, (double)V2_low_value);

        // /* VH */
        // local_gain   = shield.sensors.retrieveStoredParameterValue(V_HIGH, gain);
        // local_offset = shield.sensors.retrieveStoredParameterValue(V_HIGH, offset);
        // printk("VH: GAIN=%.4f OFFSET=%.4f RAW=%.3f\n",
        //     (double)local_gain, (double)local_offset, (double)V_high_value);

        // /* I1 */
        // local_gain   = shield.sensors.retrieveStoredParameterValue(I1_LOW, gain);
        // local_offset = shield.sensors.retrieveStoredParameterValue(I1_LOW, offset);
        // printk("I1: GAIN=%.4f OFFSET=%.4f RAW=%.3f\n",
        //     (double)local_gain, (double)local_offset, (double)I1_low_value);

        // /* I2 */
        // local_gain   = shield.sensors.retrieveStoredParameterValue(I2_LOW, gain);
        // local_offset = shield.sensors.retrieveStoredParameterValue(I2_LOW, offset);
        // printk("I2: GAIN=%.4f OFFSET=%.4f RAW=%.3f\n",
        //     (double)local_gain, (double)local_offset, (double)I2_low_value);

        // /* IH */
        // local_gain   = shield.sensors.retrieveStoredParameterValue(I_HIGH, gain);
        // local_offset = shield.sensors.retrieveStoredParameterValue(I_HIGH, offset);
        // printk("IH: GAIN=%.4f OFFSET=%.4f RAW=%.3f\n",
        //     (double)local_gain, (double)local_offset, (double)I_high_value);

        printk("Leg %d tracks %s RAW=%.3f REF=%.3f\n",
       received_leg_number,
       power_legs[received_leg_number].tracking_name,
       (double)*(power_legs[received_leg_number].tracking_var),
       (double)power_legs[received_leg_number].wRef);

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


    if (mode == IDLEMODE)
    {
        if (pwm_enable == true)
        {
            shield.power.stop(ALL);
        }
        pwm_enable = false;
    }
    else if (mode == POWERMODE)
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