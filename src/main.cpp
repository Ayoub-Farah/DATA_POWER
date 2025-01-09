/*
 *
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
 * SPDX-License-Identifier: LGPL-2.1
 */

/**
 * @brief  This file it the main entry point of the
 *         OwnTech Power API. Please check the OwnTech
 *         documentation for detailed information on
 *         how to use Power API: https://docs.owntech.org/
 *
 * @author Clément Foucher <clement.foucher@laas.fr>
 * @author Luiz Villa <luiz.villa@laas.fr>
 */

//--------------OWNTECH APIs----------------------------------
#include "TaskAPI.h"
#include "ShieldAPI.h"
#include "SpinAPI.h"

// from control library
#include "trigo.h"
#include "filters.h"
// #include "power_ac1phase.h"
#include "ScopeMimicry.h"
#include "control_factory.h"
#include "zephyr/console/console.h"
#include "singlePhaseInverter.h"
#include "sogi.h"

#define DUTY_MIN 0.1F
#define DUTY_MAX 0.9F
#define UDC_STARTUP 0.0F
//--------------SETUP FUNCTIONS DECLARATION-------------------
void setup_routine();           /* Setups the hardware and software of the system */

//--------------LOOP FUNCTIONS DECLARATION--------------------
void loop_communication_task(); // code to be executed in the slow communication task
void loop_application_task();   // Code to be executed in the background task
void loop_critical_task();     // Code to be executed in real time in the critical task

//--------------USER VARIABLES DECLARATIONS-------------------
static const uint32_t control_task_period = 100; //[us] period of the control task
static bool pwm_enable = false;            //[bool] state of the PWM (ctrl task)

uint8_t received_serial_char;

/* Measure variables */
static float32_t V1_low_value; // [V]
static float32_t V2_low_value; // [V]
static float32_t I1_low_value; // [A]
static float32_t I2_low_value; // [A]
static float32_t V_high; // [V]
static float32_t I_high; // [A]
static float32_t V_high_filt; // [V]

static float32_t Vgrid_meas; // [V]
static float32_t Igrid_meas; // [V]


static float meas_data; // temp storage meas value (ctrl task)

// static PowerAC1PhaseOutput pq_power;
// static PowerAC1PhaseParams ac_meas_config;
// static PowerAC1Phase inverter;

static singlePhaseInverter inverter;

static dqo_t power;

static float32_t Vnet;
static float32_t virtual_Vgrid_amplitude = 18.0F;
static float32_t Vq_net;

static dqo_t Vdq;
static dqo_t Vdq_ref;
static dqo_t Vdq_ref_max;
static dqo_t Vdq_ref_min;

static dqo_t Idq;
static dqo_t Idq_ref;
static dqo_t Idq_ref_max;
static dqo_t Idq_ref_min;
static dqo_t Idq_ref_delta;


static dqo_t Vdq_output;

static float32_t Id_ref_delta = 0.0;
static float32_t Iq_ref_delta = 0.0;


static float32_t Vd_ref_max = 20.0;
static float32_t Vd_ref_min = 0.0;


static clarke_t Vab;
static clarke_t Vab_output;
static clarke_t Iab;

static float32_t Vond;
static float32_t R_load = 10;

static float32_t Ialpha, Ibeta;
static const float32_t sync_power_tolerance = 0.1;
static bool is_net_synchronized;
static float32_t omega;


/* duty_cycle*/
static float32_t duty_cycle;// [No unit]

static float32_t Udc = 20.0F; // dc voltage supply assumed [V]
static const float f0 = 50.0F; // fundamental frequency [Hz]
static const float32_t w0 = 2.0F * PI * f0;   // pulsation [rad/s]
static float32_t w;   // corrected pulse [rad/s]
/* Sinewave settings */
static float32_t Vgrid_ref; //[V]
static float32_t Vgrid_amplitude_ref = 0.0F; // [V]
static float32_t Vgrid_amplitude = 0.0F; // [V]
static float angle = 0.F; // [rad]
static float theta = 0.F; // [rad]

//------------- PR RESONANT -------------------------------------
static float32_t Ts = control_task_period * 1.0e-6F;

// static float32_t kp = 0.000215;
// static float32_t Ti = 0.2*7.5175e-5;
static float32_t kp = 0.01;      // kp is 2* 66e-6 Henry/100e-3 seconds
static float32_t Ti = 0.003;      // Ti is 4*Taui = 400e-3
float32_t Td = 0.0;
float32_t N = 1.0;
float32_t upper_bound = 30;
float32_t lower_bound = -30;

static Pid pi_current_d = controlLibFactory.pid(Ts, kp, Ti, Td, N, lower_bound, upper_bound);
static Pid pi_current_q = controlLibFactory.pid(Ts, kp, Ti, Td, N, lower_bound, upper_bound);

static Pid pi_voltage_d = controlLibFactory.pid(Ts, 0.01, 0.003, Td, N, lower_bound, upper_bound);
static Pid pi_voltage_q = controlLibFactory.pid(Ts, 0.01, 0.003, Td, N, lower_bound, upper_bound);

static Pid pi_pll;
static PidParams pi_pll_params;    ///< PI controller parameters.

static float32_t rise_time = 1.0F * 2.0F * PI / w0;
static float32_t wn = 3.0F / rise_time;
static float32_t xsi = 0.7F;
static float32_t Kp_pll = 2 * wn * xsi / Udc;
static float32_t Ki_pll = (wn * wn) / Udc;
static float32_t Kr = 500.0;

Sogi sogi_i;
Sogi sogi_v;


// comes from "filters.h"
LowPassFirstOrderFilter vHighFilter(Ts, 0.1F);
static uint32_t critical_task_counter;

// the scope help us to record datas during the critical task
// its a library which must be included in platformio.ini
static ScopeMimicry scope(1024, 20);
static bool is_downloading;
static bool trigger = false;
//---------------------------------------------------------------

enum serial_interface_menu_mode // LIST OF POSSIBLE MODES FOR THE OWNTECH CONVERTER
{
    IDLEMODE = 0,
    POWERMODE=1,
    ERRORMODE=3,
    STANDBYMODE=4
};

static uint8_t mode = IDLEMODE;
static uint8_t mode_asked = IDLEMODE;
static float32_t spying_mode = 0;
static const float32_t MAX_CURRENT = 8.0F;

bool a_trigger() {
    return trigger;
}

/**
 * @brief print recorded data of the ScopeMimicry instance to console
 * we use this function in coordination with a miniterm python filter on the host side.
 * `filter_recorded_data.py` to save the data in a file and format them in float.
 *
 * @param scope
 */
void dump_scope_datas(ScopeMimicry &scope)  {
	scope.reset_dump();
    printk("begin record\n");
	while(scope.get_dump_state() != finished) {
		printk("%s", scope.dump_datas());
		task.suspendBackgroundUs(100);
	}
    printk("end record\n");
}

// UTILS FUNCTIONS FOR CONTROL
float32_t saturate(const float32_t x, float32_t min, float32_t max) {
    if (x > max) {
        return max;
    }
    if (x < min) {
        return min;
    }
    return x;
}

float32_t sign(float32_t x, float32_t tol=1e-3) {
    if (x > tol) {
        return 1.0F;
    }
    if (x < -tol) {
        return -1.0F;
    }
    return 0.0F;
}

float32_t rate_limiter(const float32_t ref, float32_t value, const float32_t rate) {
    value += Ts * rate * sign(ref - value);
    return value;
}

//--------------SETUP FUNCTIONS-------------------------------

/**
 * This is the setup routine.
 * It is used to call functions that will initialize your hardware and tasks.
 * In this example, we setup the version of the spin board and a 
 * background task. The critical task is defined but not started.
 * NOTE: It is important to follow the steps and initialize the hardware first 
 * and the tasks second. 
 */
void setup_routine()
{
    // Setup the hardware first
    shield.sensors.enableDefaultTwistSensors();

    // DISABLE DC LOW CAPACITORS
    shield.power.disconnectCapacitor(LEG1);
    shield.power.disconnectCapacitor(LEG2);

    scope.connectChannel(I1_low_value, "I1_low_value");
    scope.connectChannel(I_high, "I_High");
    scope.connectChannel(V1_low_value, "V1_low_value");
    scope.connectChannel(V2_low_value, "V2_low_value");
    scope.connectChannel(duty_cycle, "duty_cycle");

	scope.connectChannel(Idq.d, "Id");
	scope.connectChannel(Idq_ref.d, "Id_ref");

	scope.connectChannel(Vab.alpha, "Valpha");
    scope.set_delay(0.0F);
    scope.set_trigger(a_trigger);
    scope.start();

    // PR initialisation.

    // ac_meas_config.grid_voltage = 10.0;
    // ac_meas_config.w0 = w0;
    // ac_meas_config.Ts = Ts;

    // inverter.init(10.0, w0, Ts);

    sogi_v.init(500.0, Ts);
    sogi_i.init(500.0, Ts);

    Idq_ref.d = 0.0;
    Idq_ref.q = 0.0;
    Vdq_ref.d = 0.0;
    Vdq_ref.q = 0.0;

    Idq_ref_max.d = 8.0;
    Idq_ref_max.q = 1.0;
    Idq_ref_min.d = -0.1;
    Idq_ref_min.q = -0.1;

    Vdq_ref_max.d = 30.0;
    Vdq_ref_max.q = 30.0;
    Vdq_ref_min.d = -0.1;
    Vdq_ref_min.q = -0.1;

    Idq_ref_delta.d = 0.0;
    Idq_ref_delta.q = 0.0;


    // parameters of the pll pi controller
    pi_pll_params.Ts = Ts;
    pi_pll_params.Td = 0.0;
    pi_pll_params.N = 1;
    pi_pll_params.Ti = Kp_pll / Ki_pll;
    pi_pll_params.Kp = Kp_pll;
    pi_pll_params.lower_bound = -10.0F * w0;
    pi_pll_params.upper_bound = 10.0F * w0;

    pi_pll.init(pi_pll_params);
    pi_pll.reset(w0);


    // power_ac1phase_init(&ac_meas_config, 10.0, 2.0*PI*50.0, Ts);
	pi_current_d.reset();
	pi_current_q.reset();
	pi_voltage_d.reset();
	pi_voltage_q.reset();
	is_net_synchronized = false;

    /* buck voltage mode */
    shield.power.initBuck(LEG1);
    shield.power.initBoost(LEG2);

    // Then declare tasks
    uint32_t app_task_number = task.createBackground(loop_application_task);
    uint32_t com_task_number = task.createBackground(loop_communication_task);
    task.createCritical(loop_critical_task, control_task_period); // Uncomment if you use the critical task

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
            printk("|     ------- grid forming ------        |\n");
            printk("|     press i : idle mode                |\n");
            printk("|     press p : power mode               |\n");
            printk("|     press d : vdref up by 5V           |\n");
            printk("|     press c : vdref down by 5V         |\n");
            printk("|     press u : vdref up by 1V           |\n");
            printk("|     press j : vdref down by 1V         |\n");
            printk("|________________________________________|\n\n");
            //------------------------------------------------------
            break;
        case 'i':
            printk("idle mode\n");
            mode_asked = IDLEMODE;
            break;
        case 'p':
                if (!is_downloading){
                    printk("power mode\n");
                    scope.start();
                    mode_asked = POWERMODE;
                }
            break;
        case 's':
                if (!is_downloading){
                    scope.start();
                    mode_asked = STANDBYMODE;
                }
            break;
        case 'u':
				if (Idq_ref.d < Idq_ref_max.d)
				{
					Idq_ref.d += 0.1F;
				}
            break;
        case 'j':
				if (Idq_ref.d > Idq_ref_min.d)
				{
					Idq_ref.d -= 0.1F;
				}
            break;
        case 'd':
				if (Idq_ref.d < Idq_ref_max.d)
				{
					Idq_ref.d += 0.5F;
				}
            break;
        case 'c':
				if (Idq_ref.d > Idq_ref_min.d)
				{
					Idq_ref.d -= 0.5F;
				}
            break;
        case 'r':
            is_downloading = true;
            trigger = false;
            break;
        case 't':
            trigger = true;
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
/* --- STATE MACHINE --------------------------------------------------------*/
// mode is the STATE variable
// in each state we compute the transitions
switch (mode) {
        case IDLEMODE:
            if (mode_asked == POWERMODE || mode_asked == STANDBYMODE) {
                if(V_high_filt >= UDC_STARTUP){
                    mode = STANDBYMODE;
                    mode_asked = STANDBYMODE;
                }
            }
            spin.led.turnOff();
        break;
        case STANDBYMODE:
            if (is_net_synchronized) spin.led.toggle();  //blinks when synchronized
            if (mode_asked == POWERMODE && is_net_synchronized){
                mode = POWERMODE;
            }
            if (mode_asked == IDLEMODE) {
                mode = IDLEMODE;
            } 
        break;
        case POWERMODE:
            if (mode_asked == IDLEMODE) {
                mode = IDLEMODE;
            } 
            if (mode_asked == STANDBYMODE) {
                mode = STANDBYMODE;
            } 
            spin.led.turnOn();
        break;

        case ERRORMODE:
        break;
    }
    if (mode_asked == IDLEMODE) mode = IDLEMODE; // global return to idle possible
/* --- END OF STATE MACHINE -------------------------------------------------*/

    if (mode == IDLEMODE)
    {
        if (!is_downloading) {
            printk("%d:", mode);
            printk("% 7.3f:", (double)Vgrid_amplitude_ref);
            printk("% 7.3f:", (double)I1_low_value);
            printk("% 7.3f:", (double)I2_low_value);
            printk("% 7.3f:", (double)V1_low_value);
            printk("%7.3f:", (double)power.d);
            printk("%7.3f:", (double)power.q);
			printk("%7.3f:", (double)Idq_ref.d);
            printk("\n");
        } else {
            dump_scope_datas(scope);
            is_downloading = false;
        }
    }
    else
    {
	    printk("%d:", mode);
	    printk("% 6.2f:", (double)Vgrid_amplitude_ref);
	    printk("% 6.2f:", (double)Vgrid_amplitude);
	    printk("% 6.2f:", (double)V1_low_value);
	    printk("%7.3f:", (double)power.d);
	    printk("%7.3f:", (double)power.q);
		printk("%7.3f:", (double)Idq_ref.d);
		printk("%7.3f:", (double)Idq.d);
		printk("%7.3f:", (double)Idq.q);
        printk("\n");
    }
    task.suspendBackgroundMs(100);
}

/**
 * This is the code loop of the critical task
 * It is executed every 100 micro-seconds defined in the setup_software function.
 * You can use it to execute an ultra-fast code with the highest priority which cannot be interruped.
 * It is from it that you will control your power flow.
 */
void loop_critical_task()
{
    critical_task_counter++;
    // RETRIEVE MEASUREMENTS
    meas_data = shield.sensors.getLatestValue(I1_LOW);
    if (meas_data != NO_VALUE) I1_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(V1_LOW);
    if (meas_data != NO_VALUE) V1_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(V2_LOW);
    if (meas_data != NO_VALUE) V2_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(I2_LOW);
    if (meas_data != NO_VALUE) I2_low_value = meas_data;

    meas_data = shield.sensors.getLatestValue(V_HIGH);
    if (meas_data != NO_VALUE) V_high = meas_data;

    meas_data = shield.sensors.getLatestValue(I_HIGH);
    if (meas_data != NO_VALUE) I_high = meas_data;

    V_high_filt = vHighFilter.calculateWithReturn(V_high);

    Vgrid_meas = V1_low_value-V2_low_value;
    Igrid_meas = (I1_low_value-I2_low_value)/2;

    // MANAGE OVERCURRENT
    if (I1_low_value > MAX_CURRENT
        || I1_low_value < -MAX_CURRENT
        || I2_low_value > MAX_CURRENT
        || I2_low_value < -MAX_CURRENT)
    {
        mode = ERRORMODE;
    }


    if (mode == IDLEMODE || mode == ERRORMODE)
    {
        // FIRST WE STOP THE PWM
        if (pwm_enable == true)
        {
            shield.power.stop(ALL);
            spin.led.turnOff();
            pwm_enable = false;
        }
        Vgrid_amplitude = 0.F;
        duty_cycle = DUTY_MIN;
    }

    if (mode == POWERMODE || mode == STANDBYMODE)
    {     

        if(mode == STANDBYMODE){
            if (pwm_enable == true)
            {
                shield.power.stop(ALL);
                spin.led.turnOff();
                pwm_enable = false;
            }
        }

        w = w0 + pi_pll.calculateWithReturn(0, -1.0*Vdq.q);
        theta = ot_modulo_2pi(theta + w * Ts);

        // theta = ot_modulo_2pi(theta + w0 * Ts);  //grid forming code 


        Vab = sogi_v.calc(Vgrid_meas,w0);
        Iab = sogi_i.calc(Igrid_meas,w0);

        Vdq = Transform::rotation_to_dqo(Vab, theta);
        Idq = Transform::rotation_to_dqo(Iab, theta);


		if (Vdq.q < sync_power_tolerance &&
			Vdq.q > -sync_power_tolerance && 
            critical_task_counter > 1000)
		{
			is_net_synchronized = true;
		}


        if(is_net_synchronized){

            // original code
            // Idq_ref_delta.d = pi_voltage_d.calculateWithReturn(Vdq_ref.d, Vdq.d); 
            // Idq_ref_delta.q = pi_voltage_q.calculateWithReturn(Vdq_ref.q, Vdq.q); 

            // current test
            Idq_ref_delta.d = 0.0; 
            Idq_ref_delta.q = 0.0; 



            if (mode == POWERMODE){
                Vdq_output.d = pi_current_d.calculateWithReturn(Idq_ref.d + Idq_ref_delta.d, Idq.d); 
                Vdq_output.q = pi_current_q.calculateWithReturn(Idq_ref.q + Idq_ref_delta.q, Idq.q); 
            } else {
                Vdq_output.d = 0;
                Vdq_output.d = 0;
                pi_current_d.reset();
            } 
            // // original code
            // // Vdq_output.d = Vdq_output.d + Vdq_ref.d; 
            // // Vdq_output.q = Vdq_output.q + Vdq_ref.q;

            // current test
            Vdq_output.d = Vdq_output.d + Vdq.d; 
            Vdq_output.q = Vdq_output.q + Vdq.q;
            Vdq_output.o = 0.0;      
        
            Vab_output = Transform::rotation_to_clarke(Vdq_output, theta);

            Vond = Vab_output.alpha;
            duty_cycle = Vond /(2.0F * V_high_filt ) + 0.5F;

            if(mode == POWERMODE){
                if (!pwm_enable)
                {
                    shield.power.start(ALL);
                    pwm_enable = true;
                }
                shield.power.setDutyCycle(ALL, duty_cycle);
            }

        }



		// // trigger = true;
        // angle = ot_modulo_2pi(angle + w0 * Ts);
		// Vnet = virtual_Vgrid_amplitude * ot_sin(angle);
        // inverter.calculatePower(Vnet, I1_low_value);
        // Vq_net = inverter.getVdq().q;
        // Vab = inverter.getVab();
        // omega = inverter.getw();

		// if (Vq_net < sync_power_tolerance &&
		// 	Vq_net > -sync_power_tolerance && critical_task_counter > 1000)
		// {
		// 	is_net_synchronized = true;
		// }

		// if (is_net_synchronized) {
		// 	Id = inverter.getIdq().d;
		// 	Iq = inverter.getIdq().q;
		// 	Ialpha = inverter.getIab().alpha;
		// 	Ibeta = inverter.getIab().beta;

		// 	// Vdq.d = pi_current_d.calculateWithReturn(0.0, Id);
		// 	// Vdq.q = pi_current_q.calculateWithReturn(Iq_ref, Iq);


		// 	Vdq.d = Vd_ref;
		// 	Vdq.q = 0;
		// 	Vdq.o = 0.0;
		// 	Vab = Transform::rotation_to_clarke(Vdq, inverter.getTheta());
		// 	Vond = Vab.alpha;
		// 	duty_cycle = Vond /(2.0F * Udc ) + 0.5F;
		// }
		// else
		// {
		// 	duty_cycle = 0.5;

		// }
        // shield.power.setDutyCycle(ALL, duty_cycle);

    }
    if (critical_task_counter%1 == 0) {
        spying_mode = (float32_t) mode;
        scope.acquire();
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
