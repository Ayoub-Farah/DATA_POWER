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

// To build this standalone example, define USE_GFI_EXAMPLE.
#ifdef USE_GFI_EXAMPLE

//--------------OWNTECH APIs----------------------------------
#include "DataAPI.h"
#include "TaskAPI.h"
#include "TwistAPI.h"
#include "SpinAPI.h"

// from control library
#include "trigo.h"
#include "filters.h"
// #include "power_ac1phase.h"
#include "ScopeMimicry.h"
#include "control_factory.h"
#include "zephyr/console/console.h"
#include "singlePhaseInverter.h"

#define DUTY_MIN 0.1F
#define DUTY_MAX 0.9F
#define UDC_STARTUP 0.0F
#define SCOPE_MEM 1024
#define SCOPE_LENGTH 14
//--------------SETUP FUNCTIONS DECLARATION-------------------
void setup_routine(); // Setups the hardware and software of the system

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


static float meas_data; // temp storage meas value (ctrl task)

// static PowerAC1PhaseOutput pq_power;
// static PowerAC1PhaseParams ac_meas_config;
// static PowerAC1Phase inverter;

static singlePhaseInverter inverter;
static singlePhaseInverter inverter_meas;

static dqo_t power;

static float32_t Vnet;
static float32_t virtual_Vgrid_amplitude = 18.0F;
static float32_t Vgrid_amplitude = 3.0F;
static float32_t Vq_net;
static dqo_t Vdq;
static float32_t Iq_ref = 0.0;
static clarke_t Vab;
static clarke_t Vab_meas;
static float32_t Vinst;

static float32_t Vond;
static float32_t Id, Iq;
static float32_t theta;
static float32_t Ialpha, Ibeta;
static int i = 0;
static float32_t sum = 0;
static float32_t sum2 = 0;
static float32_t Vg[200];
static float32_t Vrms_classic;
static float32_t Vrms_ieee;
static float32_t Vrms_4_method;
static float32_t Vrms_ab_method;
static float32_t Vrms_2_method;

static const float32_t sync_power_tolerance = 0.1;
static bool is_net_synchronized;
static float32_t omega;

static uint8_t four_samples_counter = 0;
static bool four_samples_init = false;


static uint16_t ieee_counter_1;
static float32_t Vg_ieee_1[300];
static uint16_t ieee_counter_2;
static float32_t Vg_ieee_2[300];
static bool positive_cycle = true;


/* duty_cycle*/
static float32_t duty_cycle;// [No unit]

static float32_t Udc = 60.0F; // dc voltage supply assumed [V]
static const float f0 = 50.0F; // fundamental frequency [Hz]
static const float32_t w0 = 2.0F * PI * f0;   // pulsation [rad/s]
/* Sinewave settings */
static float32_t Vgrid; //[V]
static three_phase_t Vgrid_abc;
static clarke_t Vab_grid;
static float32_t Vgrid_ref; //[V]
static float32_t Vgrid_amplitude_ref = 0.0F; // [V]
static float angle = 0.F; // [rad]

static float32_t Vg_4_method[4];

static float32_t Ts = control_task_period * 1.0e-6F;

static float32_t kp = 0.000215;
static float32_t Ti = 0.2*7.5175e-5;
float32_t Td = 0.0;
float32_t N = 1.0;
float32_t upper_bound = Udc;
float32_t lower_bound = -Udc;
float32_t cur_up_bound = 4;
float32_t cur_low_bound = 0;
static Pid pi_voltage = controlLibFactory.pid(Ts*200, kp, Ti, Td, N, cur_low_bound, cur_up_bound);
static Pid pi_current_d = controlLibFactory.pid(Ts, kp, Ti, Td, N, lower_bound, upper_bound);
static Pid pi_current_q = controlLibFactory.pid(Ts, kp, Ti, Td, N, lower_bound, upper_bound);

// comes from "filters.h"
LowPassFirstOrderFilter vHighFilter(Ts, 0.1F);
static uint32_t critical_task_counter;

// the scope help us to record datas during the critical task
// its a library which must be included in platformio.ini
static ScopeMimicry scope(SCOPE_MEM, SCOPE_LENGTH);
static bool is_downloading;
static bool trigger = false;
//---------------------------------------------------------------

enum serial_interface_menu_mode // LIST OF POSSIBLE MODES FOR THE OWNTECH CONVERTER
{
    IDLEMODE = 0,
    POWERMODE=1,
    ERRORMODE=3,
    STARTUPMODE=4
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


/**
 * @brief Give a sign linked to a tolerance value
 *
 * @param x    signal toleranced
 * @param tol  tolerance value
 * 
 * @returns -1 if under tol, or 1 if above tol
 */
float32_t sign(float32_t x, float32_t tol=1e-3) {
    if (x > tol) {
        return 1.0F;
    }
    if (x < -tol) {
        return -1.0F;
    }
    return 0.0F;
}


/**
 * @brief Ramps up a signal at a given rate
 *
 * @param ref    signal final amplitude
 * @param value  initial value
 * @param rate   rate to reach reference
 * 
 * @returns current value ramping up.
 */
float32_t rate_limiter(const float32_t ref, float32_t value, const float32_t rate) {
    value += Ts * rate * sign(ref - value);
    return value;
}

//--------------SETUP FUNCTIONS-------------------------------

/**
 * This is the setup routine.
 * It is used to call functions that will initialize your spin, twist, data and/or tasks.
 * In this example, we setup the version of the spin board and a background task.
 * The critical task is defined but not started.
 */
void setup_routine()
{
    // Setup the hardware first
    spin.version.setBoardVersion(SPIN_v_1_0);
    twist.setVersion(shield_TWIST_V1_3);

    data.enableTwistDefaultChannels();
    // DISABLE DC LOW CAPACITORS
    spin.gpio.configurePin(PC6, OUTPUT);
    spin.gpio.configurePin(PB7, OUTPUT);
    spin.gpio.resetPin(PC6);
    spin.gpio.resetPin(PB7);
    

    // scope.connectChannel(I1_low_value, "I1_low_value");
    // scope.connectChannel(I_high, "iHigh");
    // scope.connectChannel(V1_low_value, "V1_low_value");
    // scope.connectChannel(V2_low_value, "V2_low_value");
    // scope.connectChannel(V_high_filt, "V_high_filt");
    // scope.connectChannel(duty_cycle, "duty_cycle");
    // scope.connectChannel(power.d, "power_p");
    // scope.connectChannel(power.q, "power_q");
    // scope.connectChannel(Vq_net, "Vq_net");
    scope.connectChannel(Vnet, "Vnet");
	scope.connectChannel(Vrms_classic, "Vrms_classic");
	scope.connectChannel(Vrms_ieee, "Vrms_ieee");
	scope.connectChannel(Vrms_4_method, "Vrms_4_method");
	scope.connectChannel(Vrms_2_method, "Vrms_2_method");
	scope.connectChannel(Vrms_ab_method, "Vrms_ab_method");
	scope.connectChannel(Id, "Id");
	scope.connectChannel(Iq, "Iq");
	scope.connectChannel(Iq_ref, "Iq_ref");
	scope.connectChannel(Ialpha, "Ialpha");
	scope.connectChannel(Vinst , "Vinst");
	// scope.connectChannel(Vdq.d, "Vd_ond");
	// scope.connectChannel(Vdq.q, "Vq_ond");
	// scope.connectChannel(Vab.alpha, "Valpha");
	// scope.connectChannel(Vab.beta, "Vbeta");
	scope.connectChannel(Vab_meas.alpha, "Valpha_meas");
	scope.connectChannel(Vab_meas.beta, "Vbeta_meas");
	scope.connectChannel(omega, "omega");
    scope.set_delay(0.2);
    scope.set_trigger(a_trigger);
    scope.start();

    // PR initialisation.

    // ac_meas_config.grid_voltage = 10.0;
    // ac_meas_config.w0 = w0;
    // ac_meas_config.Ts = Ts;

    inverter.init(10.0, w0, Ts);
    inverter_meas.init(10.0,w0,Ts);

    // power_ac1phase_init(&ac_meas_config, 10.0, 2.0*PI*50.0, Ts);
	pi_current_d.reset();
	pi_current_q.reset();
	is_net_synchronized = false;
    /* buck voltage mode */
    twist.initLegBuck(LEG1);
    twist.initLegBoost(LEG2);

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
            printk("|     press u : increase IQ by 0.1A      |\n");
            printk("|     press d : lower IQ by 0.1A         |\n");
            printk("|     press y : increase V  by 1V        |\n");
            printk("|     press z : lower V  by 1V           |\n");
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
        case 'u':
				if (Iq_ref < 3.0F)
				{
					Iq_ref += 0.1F;
				}
            break;
        case 'd':
				if (Iq_ref > 0.1F)
				{
					Iq_ref -= 0.1F;
				}
            break;
        case 'y':
				if (Vgrid_amplitude < 25.0F)
				{
					Vgrid_amplitude += 0.1F;
				}
            break;
        case 'z':
				if (Vgrid_amplitude > 1.0F)
				{
					Vgrid_amplitude -= 0.1F;
				}
            break;
        case 'r':
            is_downloading = true;
            trigger = false;
            break;
        case 't':
            virtual_Vgrid_amplitude = 13.4;
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
            if (mode_asked == POWERMODE && V_high_filt >= UDC_STARTUP) {
                mode = STARTUPMODE;
            }
        break;
        case STARTUPMODE:
            if (duty_cycle > 0.49F ) mode = POWERMODE;
        break;
        case POWERMODE:
            if (mode_asked == IDLEMODE) {
                mode = IDLEMODE;
            }
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
			printk("%7.3f:", (double)Iq_ref);
            printk("\n");
        } else {
            dump_scope_datas(scope);
            is_downloading = false;
        }
    }
    else
    {
	    printk("%d:", mode);
	    printk("% 6.2f:", (double)virtual_Vgrid_amplitude);
	    printk("% 6.2f:", (double)Vgrid_amplitude);
	    printk("% 6.2f:", (double)V1_low_value);
	    // printk("%7.3f:", (double)power.d);
	    // printk("%7.3f:", (double)power.q);
		printk("%7.3f:", (double)Iq_ref);
		printk("%7.3f:", (double)Vdq.d);
		printk("%7.3f:", (double)Vdq.q);
        printk("%7.3f:", (double)Vab.alpha);
        printk("%7.3f:", (double)Vab_meas.alpha);
        printk("%7.3f:", V1_low_value - V2_low_value);
        printk("%7.3f:", (double)Vrms_classic);
        printk("%7.3f:", (double)Vrms_ieee);
        printk("%7.3f:", (double)Vrms_4_method);
        printk("%7.3f:", (double)Vrms_2_method);
        printk("%7.3f:", (double)Vrms_ab_method);
        printk("%7.3f:", (double)four_samples_init);
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
    meas_data = data.getLatest(I1_LOW);
    if (meas_data != NO_VALUE) I1_low_value = meas_data;

    meas_data = data.getLatest(V1_LOW);
    if (meas_data != NO_VALUE) V1_low_value = meas_data;

    meas_data = data.getLatest(V2_LOW);
    if (meas_data != NO_VALUE) V2_low_value = meas_data;

    meas_data = data.getLatest(I2_LOW);
    if (meas_data != NO_VALUE) I2_low_value = meas_data;

    meas_data = data.getLatest(V_HIGH);
    if (meas_data != NO_VALUE) V_high = meas_data;

    meas_data = data.getLatest(I_HIGH);
    if (meas_data != NO_VALUE) I_high = meas_data;

    V_high_filt = vHighFilter.calculateWithReturn(V_high);

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
            twist.stopAll();
            spin.led.turnOff();
            pwm_enable = false;
        }
        Vgrid_amplitude = 0.F;
        duty_cycle = DUTY_MIN;
    }

    if (mode == STARTUPMODE) { // ramp up the common voltage to Udc/2
        duty_cycle = rate_limiter(0.5F, duty_cycle, 50.0F); // ramp of 50/s
        // duty_cycle = duty_cycle; // ramp of 50/s
        if (duty_cycle > 0.5F) {
            duty_cycle = 0.5F;
        }
        twist.setLegDutyCycle(LEG2, 1-duty_cycle);
        twist.setLegDutyCycle(LEG1, duty_cycle);
        // WE START THE PWM
        if (!pwm_enable)
        {
            twist.startAll();
            pwm_enable = true;
        }
    }
    if (mode == POWERMODE)
    {
		// trigger = true;
        Vinst = (V1_low_value-V2_low_value);
        angle = ot_modulo_2pi(angle + w0 * Ts);
		Vnet = virtual_Vgrid_amplitude * ot_sin(angle);
        inverter.calculatePower(Vnet, (I1_low_value-I2_low_value));
        inverter_meas.calculatePower(Vinst,(I1_low_value-I2_low_value));
        Vq_net = inverter.getVdq().q;
        Vab = inverter.getVab();
        Vab_meas = inverter_meas.getVab();
        omega = inverter_meas.getw();

		if (Vq_net < sync_power_tolerance &&
			Vq_net > -sync_power_tolerance && critical_task_counter > 1000)
		{
			is_net_synchronized = true;
		}



		if (is_net_synchronized) {


            // inverter.calculatePower((V1_low_value-V2_low_value), I1_low_value);





			 Id = inverter.getIdq().d;
			 Iq = inverter.getIdq().q;
			 Ialpha = inverter.getIab().alpha;
			 Ibeta = inverter.getIab().beta;
             theta = inverter.getTheta();


            sum = 0;

            Vrms_ab_method = sqrt( ((Vab.alpha*Vab.alpha) + (Vab.beta*Vab.beta))/2 );



            if(four_samples_init){

                four_samples_counter++;
                if(four_samples_counter == 25)  Vg_4_method[0] = Vnet;
                if(four_samples_counter == 74)  Vg_4_method[1] = Vnet;
                if(four_samples_counter == 123) Vg_4_method[2] = Vnet;
                if(four_samples_counter == 172) Vg_4_method[3] = Vnet;
                if(four_samples_counter == 200) {
                    four_samples_counter=0;
                    sum = 0;
                    sum2 = 0;
                     for (int j = 0; j < 4; j++){
                        sum += (Vg_4_method[j] * Vg_4_method[j]);
                    }
                    Vrms_4_method = sqrt(sum)/2;

                     for (int j = 0; j < 2; j++){
                        sum2 += (Vg_4_method[j] * Vg_4_method[j]);
                    }
                    Vrms_2_method = sqrt(sum2/2);


                }
             } else {
                if(theta>0 && theta<=PI/100) four_samples_init = true;
             }

            // IEEE RMS method
             if (theta>0 && theta<PI){
                if (positive_cycle == false){
                    sum = 0;
                    for (int j = 0; j < ieee_counter_1; j++){
                        sum += (Vg_ieee_1[j] * Vg_ieee_1[j])/(ieee_counter_1+ieee_counter_2);
                    }
                    for (int j = 0; j < ieee_counter_2; j++){
                        sum += (Vg_ieee_2[j] * Vg_ieee_2[j])/(ieee_counter_1+ieee_counter_2);
                    }
                    Vrms_ieee = sqrt(sum);
                    positive_cycle = true;
                    ieee_counter_1 = 0;
                }
                Vg_ieee_1[ieee_counter_1] = Vnet;
                ieee_counter_1++;
             } else {
                if (positive_cycle == true){
                    sum = 0;
                    for (int j = 0; j < ieee_counter_1; j++) 
                    {
                        sum += (Vg_ieee_1[j] * Vg_ieee_1[j])/(ieee_counter_1+ieee_counter_2);
                    }
                    for (int j = 0; j < ieee_counter_2; j++) 
                    {
                        sum += (Vg_ieee_2[j] * Vg_ieee_2[j])/(ieee_counter_1+ieee_counter_2);
                    }
                    Vrms_ieee = sqrt(sum);
                    positive_cycle = false;
                    ieee_counter_2 = 0;
                }
                Vg_ieee_2[ieee_counter_2] = Vnet;
                ieee_counter_2++;
             }





             // Classic RMS method
             if (i < 200) {
                Vg[i] = Vnet;
                i++;
             }
             else {
                sum = 0;
                for (int j = 0; j < 200; j++) 
                {
                    sum += (Vg[j] * Vg[j])/200;
                }
                Vrms_classic = sqrt(sum);
                // Iq_ref = pi_voltage.calculateWithReturn(Vgrid_amplitude, Vrms_classic*sqrt(2));
                i = 0;
             }
             
			 Vdq.d = pi_current_d.calculateWithReturn(0.0, Id);
			 Vdq.q = pi_current_q.calculateWithReturn(Iq_ref, Iq);
			 Vdq.o = 0.0;            
			 Vab = Transform::rotation_to_clarke(Vdq, theta);
			
            Vond = Vab.alpha; //***

			duty_cycle = Vond /(2.0F * Udc ) + 0.5F; //***
            //Pdq=inverter.getPower();
            //Pd=Pdq.d;
            //pid_pd.calculateWithReturn(10, Pdq.d);
		}
		else
		{
			duty_cycle = 0.5;

		}
        twist.setAllDutyCycle(duty_cycle);

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

#endif // USE_GFI_EXAMPLE
