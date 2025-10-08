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
 * @brief  This example shows how to blink the onboard LED of the Spin board.
 *
 * This file now drives a 3 kHz software carrier that is compared against
 * a 50 Hz sine reference using a lookup table in the critical task.
 */

/* --------------STANDARD LIBRARY------------------------------ */
#include <stddef.h>

/* --------------ZEPHYR---------------------------------------- */
#include <zephyr/sys/printk.h>

/* --------------OWNTECH APIs---------------------------------- */
#include "SpinAPI.h"
#include "TaskAPI.h"
#include "TwistAPI.h"

/* --------------USER HEADERS---------------------------------- */
#include "my_sin.h"

/* --------------SETUP FUNCTIONS DECLARATION------------------- */

/* Setups the hardware and software of the system */
void setup_routine();

/* --------------LOOP FUNCTIONS DECLARATION-------------------- */

/* Code to be executed in the background task */
void loop_background_task();
/* Code to be executed in real time in the critical task */
void loop_critical_task();

/* --------------USER CONSTANTS-------------------------------- */
const unsigned int CONTROL_TASK_PERIOD_US = 100U; /* 10 kHz */
const float CONTROL_TASK_FREQUENCY_HZ = 1000000.0f /
                                      (float)CONTROL_TASK_PERIOD_US;
const float CARRIER_FREQUENCY_HZ = 3000.0f;
const float SINE_FREQUENCY_HZ = 50.0f;
const float ONE_THIRD_F = 0.3333333f;

/* --------------USER VARIABLES DECLARATIONS------------------- */
float carrier_value = 0.0f;
float carrier_step = 0.0f;
float sine_position = 0.0f;
float sine_step = 0.0f;

volatile unsigned int output_a = 0U;
volatile unsigned int output_b = 0U;
volatile unsigned int output_c = 0U;

volatile float last_carrier_a = 0.0f;
volatile float last_carrier_b = 0.0f;
volatile float last_carrier_c = 0.0f;
volatile float last_sine_value = 0.0f;

/* --------------SETUP FUNCTIONS------------------------------- */

/**
 * This is the setup routine.
 * It is used to call functions that will initialize your spin, power shields
 * and tasks.
 */
void setup_routine()
{
    spin.version.setBoardVersion(TWIST_v_1_1_2);

    carrier_step = CARRIER_FREQUENCY_HZ / CONTROL_TASK_FREQUENCY_HZ;
    sine_step = ((float)MY_SIN_TABLE_SIZE * SINE_FREQUENCY_HZ) /
                CONTROL_TASK_FREQUENCY_HZ;
    carrier_value = 0.0f;
    sine_position = 0.0f;

    unsigned int background_task_number = task.createBackground(loop_background_task);
    task.createCritical(loop_critical_task, CONTROL_TASK_PERIOD_US);

    task.startBackground(background_task_number);
    task.startCritical();
}

/* --------------LOOP FUNCTIONS-------------------------------- */

/**
 * This is the code loop of the background task
 * It runs perpetually. Here a `suspendBackgroundMs` is used to pause during
 * 1000ms between each LED toggles.
 */
void loop_background_task()
{
    if (output_a != 0U)
    {
        spin.led.turnOn();
    }
    else
    {
        spin.led.turnOff();
    }

    printk("sine=%.3f carrierA=%.3f carrierB=%.3f carrierC=%.3f | outA=%u outB=%u outC=%u\n",
           (double)last_sine_value,
           (double)last_carrier_a,
           (double)last_carrier_b,
           (double)last_carrier_c,
           output_a,
           output_b,
           output_c);

    task.suspendBackgroundMs(1000);
}

/**
 * This is the code loop of the critical task.
 * It runs at 10 kHz and compares the sine reference to the carriers.
 */
void loop_critical_task()
{
    unsigned int sine_index = (unsigned int)sine_position;
    if (sine_index >= MY_SIN_TABLE_SIZE)
    {
        sine_index = 0U;
    }
    float sine_reference = my_sin_table[sine_index];

    carrier_value = carrier_value + carrier_step;
    if (carrier_value >= 1.0f)
    {
        carrier_value = carrier_value - 1.0f;
    }

    float carrier_a = carrier_value;
    float carrier_b = carrier_a + ONE_THIRD_F;
    if (carrier_b >= 1.0f)
    {
        carrier_b = carrier_b - 1.0f;
    }

    float carrier_c = carrier_a + (2.0f * ONE_THIRD_F);
    if (carrier_c >= 1.0f)
    {
        carrier_c = carrier_c - 1.0f;
    }

    if (sine_reference > carrier_a)
    {
        output_a = 1U;
    }
    else
    {
        output_a = 0U;
    }

    if (sine_reference > carrier_b)
    {
        output_b = 1U;
    }
    else
    {
        output_b = 0U;
    }

    if (sine_reference > carrier_c)
    {
        output_c = 1U;
    }
    else
    {
        output_c = 0U;
    }

    last_sine_value = sine_reference;
    last_carrier_a = carrier_a;
    last_carrier_b = carrier_b;
    last_carrier_c = carrier_c;

    sine_position = sine_position + sine_step;
    if (sine_position >= (float)MY_SIN_TABLE_SIZE)
    {
        sine_position = sine_position - (float)MY_SIN_TABLE_SIZE;
    }
}

/* --------------MAIN FUNCTION-------------------------------- */

/**
 * This is the main function of this example
 * This function is generic and does not need editing.
 */
int main(void)
{
    setup_routine();

    return 0;
}
