/*
 * Copyright (c) 2021-2024 LAAS-CNRS
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
 * SPDX-License-Identifier: LGLPV2.1
 */

/**
 * @brief  This file it the main entry point of the
 *         OwnTech Power API. Please check the OwnTech
 *         documentation for detailed information on
 *         how to use Power API: https://docs.owntech.org/
 *
 * @author Clément Foucher <clement.foucher@laas.fr>
 * @author Luiz Villa <luiz.villa@laas.fr>
 * @author Ayoub Farah Hassan <ayoub.farah-hassan@laas.fr>
 */

//--------------OWNTECH APIs----------------------------------
#include "DataAPI.h"
#include "TaskAPI.h"
#include "TwistAPI.h"
#include "SpinAPI.h"
#include "opalib_control_pid.h"
#include "filters.h"

#include "zephyr/console/console.h"
#include "math.h"

/* ADC parameter */
#define TEMP_LEG1 EXTRA_MEAS
#define TEMP_LEG2 TEMP_SENSOR

/* Temperature computing parameter */
#define VREF 2.048f // voltage reference from ADC
#define QUANTUM_MAX 4096.0f // ADC resolution 
#define R_DIVIDOR 20000.0f // Resistor in the voltage divider
#define Vin_dividor 3.3f // Input voltage in the voltage divider
#define R0_temp_sensor 10000.0f // R0 parameter of the termistor
#define T0_temp_sensor 298.15f // 25°C = 298.15K
#define B_temp_sensor 3450.0f // B parameter of the termistor

//--------------SETUP FUNCTIONS DECLARATION-------------------
void setup_routine(); // Setups the hardware and software of the system

//--------------LOOP FUNCTIONS DECLARATION--------------------
void loop_communication_task(); // code to be executed in the slow communication task
void loop_application_task();   // Code to be executed in the background task
void loop_critical_task();     // Code to be executed in real time in the critical task

//--------------USER VARIABLES DECLARATIONS-------------------

static uint32_t control_task_period = 100; //[us] period of the control task
static bool pwm_enable = false;            //[bool] state of the PWM (ctrl task)

uint8_t received_serial_char;

/* Measure variables */

static float32_t V1_low_value;
static float32_t V2_low_value;
static float32_t I1_low_value;
static float32_t I2_low_value;
static float32_t I_high;
static float32_t V_high;
static float32_t temp_meas1;
static float32_t temp_meas2;

static float meas_data; // temp storage meas value (ctrl task)

float32_t duty_cycle = 0.3;

static float32_t voltage_reference = 25; //voltage reference

/* PID coefficient for a 8.6ms step response*/

static float32_t kp = 0.000215;
static float32_t ki = 2.86;
static float32_t kd = 0.0;

LowPassFirstOrderFilter filter_temp1(100e-3, 10); // filter for temp1 measure
LowPassFirstOrderFilter filter_temp2(100e-3, 10); // filter for temp2 measure

//---------------------------------------------------------------


enum serial_interface_menu_mode // LIST OF POSSIBLE MODES FOR THE OWNTECH CONVERTER
{
    IDLEMODE = 0,
    POWERMODE
};

uint8_t mode = IDLEMODE;

//--------------SETUP FUNCTIONS-------------------------------

float32_t temp_degree(float32_t voltage_quantum)
{
float32_t V_adc = (voltage_quantum/QUANTUM_MAX)*VREF;
float32_t R_t = ((V_adc/Vin_dividor)/(1 - V_adc/Vin_dividor))*R_DIVIDOR;
float32_t T = T0_temp_sensor/(1 + log(R_t/R0_temp_sensor)*(T0_temp_sensor/B_temp_sensor));
return T - 273.15f; // return value in degree
}

/**
 * This is the setup routine.
 * It is used to call functions that will initialize your spin, twist, data and/or tasks.
 * In this example, we setup the version of the spin board and a background task.
 * The critical task is defined but not started.
 */
void setup_routine()
{
    // Setup the hardware first
    spin.version.setBoardVersion(TWIST_v_1_1_2);
    twist.setVersion(shield_TWIST_V1_3);


    /* buck voltage mode */
    twist.initAllBuck();

    data.enableTwistDefaultChannels();
    data.enableShieldChannel(4, TEMP_LEG1);
    data.enableShieldChannel(3, TEMP_LEG2);

    opalib_control_init_interleaved_pid(kp, ki, kd, control_task_period);

    // Then declare tasks
    uint32_t app_task_number = task.createBackground(loop_application_task);
    uint32_t com_task_number = task.createBackground(loop_communication_task);
    task.createCritical(loop_critical_task, 100); // Uncomment if you use the critical task

    // Finally, start tasks
    task.startBackground(app_task_number);
    task.startBackground(com_task_number);
    task.startCritical(); // Uncomment if you use the critical task
}

//--------------LOOP FUNCTIONS--------------------------------

void loop_communication_task()
{
    while (1)
    {
        received_serial_char = console_getchar();
        switch (received_serial_char)
        {
        case 'h':
            //----------SERIAL INTERFACE MENU-----------------------
            printk(" ________________________________________\n");
            printk("|     ------- MENU ---------             |\n");
            printk("|     press i : idle mode                |\n");
            printk("|     press s : serial mode              |\n");
            printk("|     press p : power mode               |\n");
            printk("|     press u : duty cycle UP            |\n");
            printk("|     press d : duty cycle DOWN          |\n");
            printk("|________________________________________|\n\n");
            //------------------------------------------------------
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
}

/**
 * This is the code loop of the background task
 * It is executed second as defined by it suspend task in its last line.
 * You can use it to execute slow code such as state-machines.
 */
void loop_application_task()
{
    data.triggerAcquisition(3);
    meas_data = data.getLatest(TEMP_LEG2);
    if (meas_data != NO_VALUE){
        float32_t meas_filter = filter_temp1.calculateWithReturn(meas_data);
        temp_meas2 = temp_degree(meas_filter);
    }

    data.triggerAcquisition(4);
    meas_data = data.getLatest(TEMP_LEG1);
    if (meas_data != NO_VALUE)
    {
        float32_t meas_filter = filter_temp2.calculateWithReturn(meas_data);
        temp_meas1 = temp_degree(meas_filter);
    }

    if (mode == IDLEMODE)
    {
        spin.led.turnOff();
    }
    else if (mode == POWERMODE)
    {
        spin.led.turnOn();

        printk("%f:", temp_meas1);
        printk("%f:", temp_meas2);
        printk("%f:", I1_low_value);
        printk("%f:", V1_low_value);
        printk("%f:", I2_low_value);
        printk("%f:", V2_low_value);
        printk("%f:", I_high);
        printk("%f\n", V_high);
    }
    task.suspendBackgroundMs(100);
}

/**
 * This is the code loop of the critical task
 * It is executed every 500 micro-seconds defined in the setup_software function.
 * You can use it to execute an ultra-fast code with the highest priority which cannot be interruped.
 * It is from it that you will control your power flow.
 */
void loop_critical_task()
{
    meas_data = data.getLatest(I1_LOW);
    if (meas_data < 10000 && meas_data > -10000)
        I1_low_value = meas_data;

    meas_data = data.getLatest(V1_LOW);
    if (meas_data != -10000)
        V1_low_value = meas_data;

    meas_data = data.getLatest(V2_LOW);
    if (meas_data != -10000)
        V2_low_value = meas_data;

    meas_data = data.getLatest(I2_LOW);
    if (meas_data < 10000 && meas_data > -10000)
        I2_low_value = meas_data;

    meas_data = data.getLatest(I_HIGH);
    if (meas_data < 10000 && meas_data > -10000)
        I_high = meas_data;

    meas_data = data.getLatest(V_HIGH);
    if (meas_data != -10000)
        V_high = meas_data;

    if (mode == IDLEMODE)
    {
        if (pwm_enable == true)
        {
            twist.stopAll();
        }
        pwm_enable = false;
    }
    else if (mode == POWERMODE)
    {
        duty_cycle = opalib_control_interleaved_pid_calculation(voltage_reference, V1_low_value); // to work with LEG1
        // duty_cycle = opalib_control_interleaved_pid_calculation(voltage_reference, V2_low_value); // to work with LEG2
        twist.setAllDutyCycle(duty_cycle);

        /* Set POWER ON */
        if (!pwm_enable)
        {
            pwm_enable = true;
            twist.startLeg(LEG1); // to work with lEG1
            // twist.startLeg(LEG2); // to work with LEG2
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
