/*
 * Copyright (c) 2024-present LAAS-CNRS
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
 * @brief  This file is an example of a Field Oriented Control for
 *         OwnTech OwnVerter board.
 *         Please check example documentation to get more details
 *         how to use this example: https://docs.owntech.org/examples/
 *
 * @author Régis Ruelland <regis.ruelland@laas.fr>
 * @author Jean Alinei <jean.alinei@laas.fr>
 */

/* --------------OWNTECH APIs---------------------------------- */

#include "ScopeMimicry.h"
#include "SpinAPI.h"
#include "TaskAPI.h"
#include "ShieldAPI.h"
#include "arm_math_types.h"
#include "control_factory.h"
#include "transform.h"
#include "trigo.h"
#include "zephyr/console/console.h"

/* --------------SETUP FUNCTIONS DECLARATION------------------- */

/* Setups the hardware and software of the system */
void setup_routine();

/* --------------LOOP FUNCTIONS DECLARATION-------------------- */

/* Code to be executed in the background task */
void loop_background_task();
/* Code to be executed in real time in the critical */
void loop_critical_task();
void application_task();

/* --------------USER VARIABLES DECLARATIONS------------------- */
#define HALL1 PA7
#define HALL2 PC6
#define HALL3 PD2

static const float32_t AC_CURRENT_LIMIT = 10.0;
static const float32_t DC_CURRENT_LIMIT = 3.0;

/* Used in the control algorithm */
static const float32_t MIN_DC_VOLTAGE = 30.0F;
/* Used as a threshold to start POWER mode */
static const float32_t V_HIGH_MIN = 5.0;
static const float32_t Ts = 100.e-6F;
static const uint32_t control_task_period = (uint32_t)(Ts * 1.e6F);

/* Hall effect sensors */
static uint8_t HALL1_value;
static uint8_t HALL2_value;
static uint8_t HALL3_value;

static uint8_t angle_index;
static float32_t hall_angle;
static PllAngle pllangle = controlLibFactory.pllAngle(Ts, 10.0F, 0.04F);
static PllDatas pllDatas;
static float32_t angle_filtered;
static float32_t w_meas;

/*
 * One sector for one index value
 * Index = H1*2^0 + H2*2^1 + H3*2^2
 */
static int16_t sector[] = {-1, 5, 1, 0, 3, 4, 2};
static float32_t k_angle_offset = 0.0F;

/* Power LEG measures */
static float32_t meas_data;
static float32_t I1_low_value;
static float32_t I2_low_value;
static float32_t I1_offset;
static float32_t I2_offset;
static float32_t tmpI1_offset;
static float32_t tmpI2_offset;
static const float32_t NB_OFFSET = 2000.0;
static float32_t V1_low_value;
static float32_t V2_low_value;
static float32_t V12_value;

/* DC measures */
static float32_t I_high;
static float32_t V_high;

/* Three phase system and Park DQ Frame (dqo) */
static three_phase_t Vabc;
static three_phase_t duty_abc;
static three_phase_t Iabc;
static dqo_t Vdq;
static dqo_t Idq;
static dqo_t Idq_ref;
static float32_t angle_4_control;
static float32_t v_ref;
static float32_t Vmin;
static float32_t Vmax;
static float32_t compensation;


/* Variables used to get static value for ScopeMimicry */
static three_phase_t Iabc_ref;
static float32_t duty_a, duty_b;
static float32_t Ia_ref;
static float32_t Ib_ref;
static float32_t Va;
static float32_t Iq_meas;
static float32_t Iq_ref;
static float32_t Iq_max;
static float32_t Vd, Vq;
static float32_t HALL1_value_f;
static float32_t HALL2_value_f;
static float32_t HALL3_value_f;
static float32_t angle_index_f;

/* We only make torque control. */
static float32_t manual_Iq_ref;

static uint32_t encoder_value_raw=0;
static float32_t encoder_value_raw_float=0;
static float32_t encoder_value_minus_offset_float=0;
static int32_t encoder_value=0;
static uint32_t encoder_offset=0;
static float32_t encoder_value_max=4000.0;
static float32_t encoder_normalized=0.0;
static uint32_t previous_encoder_value=0;
static float32_t n_poles=8;
/**
 * Low Pass Filters Init
 */
static LowPassFirstOrderFilter vHigh_filter =
						controlLibFactory.lowpassfilter(Ts, 5.0e-3F);

static LowPassFirstOrderFilter w_mes_filter =
						controlLibFactory.lowpassfilter(Ts, 5.0e-3F);

static float32_t V_high_filtered;
static float32_t inverse_Vhigh;

/**
 *  PID   
 * Ki = Kp/Ti
 * Ki = 62
 */

 static float32_t Kp = 0.35;
 static float32_t Ti = 0.005;
 static float32_t Td = 0.0F;
 static float32_t N = 1.0; 

/* Coefficient 0.4 comes from Va_max =  (α_max - 0.5) * Udc     */
static float32_t lower_bound = -MIN_DC_VOLTAGE * 0.4;
static float32_t upper_bound = MIN_DC_VOLTAGE * 0.4;
static Pid pi_d = controlLibFactory.pid(Ts, Kp, Ti, Td, N,
										lower_bound, upper_bound);

static Pid pi_q = controlLibFactory.pid(Ts, Kp, Ti, Td, N,
										lower_bound, upper_bound);


/* Decimation is used to limit the rate of measurement plotted in ScopeMimicry*/
const static uint32_t decimation = 10;
static uint32_t counter_time;
float32_t counter_time_f;
static float32_t w_estimate;
uint8_t received_serial_char;

/* List of possible modes for the OwnTech power shield */
enum serial_interface_menu_mode
{
	IDLEMODE = 0,
	POWERMODE = 1,
};

/* List of possible control states */
enum control_state_mode {
	OFFSET_ST = 0,
	IDLE_ST = 1,
	POWER_ST = 2,
	ERROR_ST = 3
};

enum control_state_mode control_state;
static float32_t control_state_f;

static uint16_t error_counter;
static bool pwm_enable;
uint8_t asked_mode = IDLEMODE;

/**
 *  Definition of variables and functions for plotting real time values
 *  using ScopeMimicry.
 */

const uint16_t SCOPE_SIZE = 512;
uint16_t k_app_idx;
ScopeMimicry scope(SCOPE_SIZE, 9);
static bool is_downloading;
static bool memory_print;

/* ---------------  Example specific functions --------------------------- */

bool mytrigger()
{
	return (control_state == POWER_ST);
}

void dump_scope_datas(ScopeMimicry &scope) {
	printk("begin record\n");
	scope.reset_dump();
	while (scope.get_dump_state() != finished) {
		printk("%s", scope.dump_datas());
		task.suspendBackgroundUs(200);
	}
	printk("end record\n");
}

/**
 * @brief An initialization function for VHigh measurement, the pll, the pid
 *        and the error counter.
 */
void init_filt_and_reg(void)
{
	pllangle.reset(0.F);
	vHigh_filter.reset(V_HIGH_MIN);
	pi_d.reset();
	pi_q.reset();
	error_counter = 0;
}

/**
 * @brief A period-meter function which estimate pulsation
 * for the sector variable (one integer value correspond to π/3).
 *
 * @param sector assume sector is integer in [0, 5]
 * @param time in [s]
 * @return pulsation (float)
 */
float32_t pulsation_estimator(int16_t sector, float32_t time)
{
	static float32_t w_estimate_intern = 0.0F;
	static int16_t prev_sector;
	static float32_t prev_time = 0.0F;
	int16_t delta_sector;
	float32_t sixty_degre_step_time;
	delta_sector = sector - prev_sector;
	prev_sector = sector;
	if (delta_sector == 1 || delta_sector == -5) {
		/* positive speed */
		sixty_degre_step_time = (time - prev_time);
		w_estimate_intern = (PI / 3.0) / sixty_degre_step_time;
		prev_time = time;
	}
	if (delta_sector == -1 || delta_sector == 5) {
		sixty_degre_step_time = (time - prev_time);
		w_estimate_intern = (-PI / 3.0) / sixty_degre_step_time;
		prev_time = time;
	}
	return w_estimate_intern;
}

/**
 *  @brief Function that retrieves all measurements from power shield sensors.
 */
inline void retrieve_analog_datas()
{
	meas_data = shield.sensors.getLatestValue(I1_LOW);
	if (meas_data != NO_VALUE) {
		I1_low_value = meas_data + I1_offset;
	}

	meas_data = shield.sensors.getLatestValue(I2_LOW);
	if (meas_data != NO_VALUE) {
		I2_low_value = meas_data + I2_offset;
	}

	if (control_state == OFFSET_ST && counter_time < NB_OFFSET) {
		tmpI1_offset += I1_low_value;
		tmpI2_offset += I2_low_value;
	}

	meas_data = shield.sensors.getLatestValue(V_HIGH);
	if (meas_data != NO_VALUE) {
		V_high = meas_data;
	}

	meas_data = shield.sensors.getLatestValue(I_HIGH);
	if (meas_data != NO_VALUE) {
		/* Sign is negative because of the way hardware sensor is routed */
		I_high = -meas_data;
	}

	meas_data = shield.sensors.getLatestValue(V1_LOW);
	if (meas_data != NO_VALUE) {
		V1_low_value = meas_data;
	}

	meas_data = shield.sensors.getLatestValue(V2_LOW);
	if (meas_data != NO_VALUE) {
		V2_low_value = meas_data;
	}

	/* Vhigh measurement gets additional filtering */
	V_high_filtered = vHigh_filter.calculateWithReturn(V_high);

	V12_value = V1_low_value - V2_low_value;
}

/**
 * @brief Reads the encoder pins and estimate position and speed.
 */
inline void get_position_and_speed()
{
	/* We encoder vales are read from timer 3 */
    encoder_value_raw = spin.timer.getIncrementalEncoderValue(TIMER3);
    encoder_value_raw_float = (float)encoder_value_raw;
	encoder_value = encoder_value_raw ; 
    encoder_value_minus_offset_float = (float)encoder_value;

    /* Encoder value is then normalized to find the angle */
    encoder_normalized = n_poles*(encoder_value_raw_float/encoder_value_max);
	float32_t angle_calculated = ot_modulo_2pi(2*PI*encoder_normalized); 
	if (angle_calculated < 2*PI){
    	angle_filtered = angle_calculated;
	}


    /* This is a test code to auto-generate the angle without the encoder */
	// hall_angle = ot_modulo_2pi(PI / 3.0 * sector[angle_index] + 
    //              PI * k_angle_offset / 24.0);

	// w_estimate = pulsation_estimator(sector[angle_index], counter_time * Ts);
    // w_estimate = 
	// pllDatas = pllangle.calculateWithReturn(hall_angle);

	// angle_filtered = pllDatas.angle;
	// w_meas = w_mes_filter.calculateWithReturn(pllDatas.w);

    /* Angle value is saved for next cycle */
    previous_encoder_value = encoder_value;
}

/**
 * @brief Handles current limits and switch to Error state if limits exceeded.
 */
inline void overcurrent_mngt()
{
	if (I1_low_value > AC_CURRENT_LIMIT || I1_low_value < -AC_CURRENT_LIMIT ||
	    I2_low_value > AC_CURRENT_LIMIT || I2_low_value < -AC_CURRENT_LIMIT ||
	    I_high > DC_CURRENT_LIMIT) {
		error_counter++;
	}
	if (error_counter > 2) {
		control_state = ERROR_ST;
	}
}

/**
 * @brief Stops PWM and reset filter and PID states
 */
inline void stop_pwm_and_reset_states_ifnot()
{
	if (pwm_enable == true) {
		shield.power.stop(ALL);
		/* Reset filters and pid */
		init_filt_and_reg();
		pwm_enable = false;
	}
}

/**
 * @brief Performs Torque control using Field Oriented Control algorithm
 */
inline void control_torque()
{
    /* Gathers the angle and references */
	angle_4_control = angle_filtered;
	Idq_ref.q = manual_Iq_ref;
	Idq_ref.d = manual_Iq_ref;

	/* Saturation */
	if (Idq_ref.q > Iq_max) {
		Idq_ref.q = Iq_max;
	}
	if (Idq_ref.q < -Iq_max) {
		Idq_ref.q = -Iq_max;
	}

    /* Gets the current measurements */
	Iabc.a = I1_low_value;
	Iabc.b = I2_low_value;
	Iabc.c = -(Iabc.a + Iabc.b);

	Idq = Transform::to_dqo(Iabc, angle_4_control);
	Vdq.d = pi_d.calculateWithReturn(Idq_ref.d, Idq.d);
	Vdq.q = pi_q.calculateWithReturn(Idq_ref.q, Idq.q);
	Vdq.o = 0.0F;

    /* Manual override to test in open loop */
	// Vdq.d = v_ref;
	// Vdq.q = v_ref;
	// Vdq.o = 0.0F;

	Vabc = Transform::to_threephase(Vdq, angle_4_control);
}

/**
 * @brief Helper function that computes duty cycles from ABC frame.
 */
inline void compute_duties()
{
	// inverse_Vhigh = 1.0 / V_high_filtered;
    
    // Vmin = Vabc.a;
    // Vmax = Vabc.a;

    // if(Vmin>Vabc.b) Vmin = Vabc.b;
    // if(Vmin>Vabc.c) Vmin = Vabc.c;
    // if(Vmax<Vabc.b) Vmax = Vabc.b;
    // if(Vmax<Vabc.c) Vmax = Vabc.c;

    // /* This applies the third harmonic injection */
    // compensation = 0.5 * (1 - ((Vmin+Vmax)*inverse_Vhigh) );
    // compensation = 0.5 * (1 - ((Vmin+Vmax)*inverse_Vhigh) );
    
	// duty_abc.a = (Vabc.a * inverse_Vhigh + compensation);
	// duty_abc.b = (Vabc.b * inverse_Vhigh + compensation);
	// duty_abc.c = (Vabc.c * inverse_Vhigh + compensation);
	inverse_Vhigh = 1.0 / MIN_DC_VOLTAGE;
	duty_abc.a = (Vabc.a * inverse_Vhigh + 0.5);
	duty_abc.b = (Vabc.b * inverse_Vhigh + 0.5);
	duty_abc.c = (Vabc.c * inverse_Vhigh + 0.5);
}

/**
 * @brief Helper function that sends the duty cycles to the shield.
 */
inline void apply_duties()
{
	shield.power.setDutyCycle(LEG1, duty_abc.a);
	shield.power.setDutyCycle(LEG2, duty_abc.b);
	shield.power.setDutyCycle(LEG3, duty_abc.c);
}

/**
 * @brief Helper function to start the power shield PWMs
 */
void start_pwms_ifnot()
{
	if (!pwm_enable) {
		pwm_enable = true;
		shield.power.start(ALL);
	}
}

/**
 * @brief Initialization function for the example variables
 */
void init_variables()
{
	/* Time counter */
	counter_time = 0;
	/* Measurements variables */
	I1_low_value = 0.0F;
	I2_low_value = 0.0F;
	I_high = 0.0F;
	V_high = 0.0F;
	/* Offset variables */
	I1_offset = 0.0F;
	I2_offset = 0.0F;
	tmpI1_offset = 0.0F;
	tmpI2_offset = 0.0F;
	/* State view of the pwm */
	pwm_enable = false;
	/* Idle or power mode*/
	asked_mode = IDLEMODE;
	/* We begin to measure the current offset before all */
	control_state = OFFSET_ST;
	Iq_max = 2.0;
	manual_Iq_ref = 0.0F;
}
/* ------------END of Example specific functions --------------------------- */



/* --------------SETUP FUNCTIONS------------------------------- */

/**
 * In this setup routine :
 *  - Power shield is initialized
 * 		- Shield is set in Buck Mode.
 * 		- Default sensors are activated
 * 		- GPIOs are set for Hall effect sensor
 *  - ScopeMimicry is initialized
 * 	- VHigh filter and PIDs are initialized
 * 	- LED is turned on.
 *  - Tasks are initialized and started
 */
void setup_routine()
{
	/* Setup the hardware first */
	shield.power.initBuck(ALL);
	shield.sensors.enableDefaultOwnverterSensors();

    shield.sensors.setConversionParametersLinear(V1_LOW,0.0513937134094877,-99.81831671778261   );
    shield.sensors.setConversionParametersLinear(V2_LOW,0.05176364848841735,-100.18487372694851 );
    shield.sensors.setConversionParametersLinear(V3_LOW,0.05130754448322653,-99.90662375911359  );
    shield.sensors.setConversionParametersLinear(V_HIGH,0.028585591456274668,2.564582060162749  );
    shield.sensors.setConversionParametersLinear(I1_LOW,0.01011000917925166,-20.194898856128685 );
    shield.sensors.setConversionParametersLinear(I2_LOW,0.009573005525177453,-18.95900696851234 );
    shield.sensors.setConversionParametersLinear(I3_LOW,0.00988678047895755,-19.544379045764682 );
    shield.sensors.setConversionParametersLinear(I_HIGH,-0.009624807234262563,19.232911369709534);

    /* Initializes the incremental encoder logging */
    spin.timer.startLogIncrementalEncoder(TIMER3);

	/* Scope configuration - Please verify with line 216 */
	scope.connectChannel(Vq, "Vq");                         /* 1 */
	scope.connectChannel(Vd, "Vd");                         /* 2 */
	scope.connectChannel(duty_abc.a, "duty_a");             /* 3 */
	scope.connectChannel(duty_abc.b, "duty_b");             /* 4 */
	scope.connectChannel(duty_abc.c, "duty_c");             /* 5 */
	scope.connectChannel(I2_low_value, "I2_low_value");     /* 6 */
	scope.connectChannel(I_high, "I_high_value");     	    /* 7 */
	// scope.connectChannel(Iq_meas, "Iq_meas");               /* 8 */
	scope.connectChannel(encoder_value_raw_float, "encoder_raw"); /* 9 */
	scope.connectChannel(angle_filtered, "encoder_calculated"); /* 9 */
	scope.set_trigger(&mytrigger);
	scope.set_delay(0.0);
	scope.start();

	/* Initialize values */
	init_filt_and_reg();
	init_variables();
	spin.led.turnOn();

	/* Declare tasks */
	uint32_t background_task_number =
					task.createBackground(loop_background_task);

	uint32_t app_task_number = task.createBackground(application_task);
	task.createCritical(loop_critical_task, control_task_period);

	/* Finally, start tasks */
	task.startBackground(background_task_number);
	task.startBackground(app_task_number);
	task.startCritical();
}

/* --------------LOOP FUNCTIONS-------------------------------- */

/**
 * This background task retrieve user inputs to control the OwnVerter:
 * - p and I keys respectively Power ON and Power OFF the inverter
 * - u and D keys respectively Increase and Decrease Iq reference.
 * - r Q and M keys are used to control ScopeMimicry data retrieval.
 */
void loop_background_task()
{
	received_serial_char = console_getchar();
	switch (received_serial_char) {
    case 'h':
        /* ----------SERIAL INTERFACE MENU----------------------- */
        printk(" ___________________________________________ \n"
               "|     ------- MENU ---------                |\n"
               "|     press p : ask for power               |\n"
               "|     press i : ask for idle                |\n"
               "|     press r : retrieve scope data         |\n"
               "|     press u : UP Iq Ref (default +0.1A)   |\n"
               "|     press d : DOWN Iq Ref (default -0.1A) |\n"
               "|     press m : print scope data on ownplot |\n"
               "|     press q : reset scope acquisition     |\n"
               "|___________________________________________|\n\n");
        /* ------------------------------------------------------ */        
	case 'p':
		printk("power asked");
		asked_mode = POWERMODE;
		scope.start();
		break;
	case 'i':
		printk("idle asked");
		asked_mode = IDLEMODE;
		break;
	case 'r':
		is_downloading = true;
	case 'u':
		manual_Iq_ref += 0.1F;
		break;
	case 'd':
        manual_Iq_ref -= 0.1F;
		break;
	case 'm':
		/* To print scope datas in ownplot as soon as possible */
		memory_print = !memory_print;
		break;
	case 'q':
		/* Restart scope acquisition */
		scope.start();
		break;
	}
}

/**
 * This application task sends data over USB Serial.
 */
void application_task()
{
	if (!memory_print) {
		printk("%7.2f", V_high);
		// printk("%7.2f:",v_ref);
		// printk("%7.2f:", Iq_max);
		printk("%7.2f:", manual_Iq_ref);
		// printk("%7.2f:", I1_offset);
		printk("%7d:", control_state);
		printk("%7d:", error_counter);
		printk("%7f:", I1_low_value);
		printk("%7f:", I2_low_value);
		printk("%7.2f", I_high);
		// printk("%7d:", encoder_value);
		// printk("%7.2f:", angle_filtered);
		printk("\n");
	} else {
		/* If memory_print is true then we plot scope datas in an infinite loop
		 * This can be used with ownplot if you have not python script installed
		 * to plot downloaded data using dump_scope_datas().
		 */
		k_app_idx = (k_app_idx + 1) % SCOPE_SIZE;
		printk("%.2f:", scope.get_channel_value(k_app_idx, 0));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 1));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 2));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 3));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 4));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 5));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 6));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 7));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 8));
		printk("%.2f:", scope.get_channel_value(k_app_idx, 9));
		printk("\n");
	}

	if (is_downloading) {
		dump_scope_datas(scope);
		is_downloading = false;
	}
	switch (control_state) {
	case OFFSET_ST:
		if (counter_time > (uint32_t)NB_OFFSET) {
			spin.led.turnOff();
			I1_offset = -tmpI1_offset / NB_OFFSET;
			I2_offset = -tmpI2_offset / NB_OFFSET;
			control_state = IDLE_ST;
		}
		break;

	case IDLE_ST:
		if ((asked_mode == POWERMODE) && (V_high_filtered > V_HIGH_MIN)) {
			control_state = POWER_ST;
		}
		break;

	case POWER_ST:
		if (asked_mode == IDLEMODE) {
			control_state = IDLE_ST;
		}
		break;

	case ERROR_ST:
		if (asked_mode == IDLEMODE) {
			error_counter = 0;
			control_state = IDLE_ST;
		}
		break;
	}

	task.suspendBackgroundMs(250);
}


/**
 * This is the critical task that runs at 10kHz.
 * It performs the Field Oriented Control.
 */
void loop_critical_task()
{
	counter_time++;

	retrieve_analog_datas();

	get_position_and_speed();

	// overcurrent_mngt();  // issues with calculation during testing

	switch (control_state) {
	case OFFSET_ST:
		stop_pwm_and_reset_states_ifnot();
		break;
	case IDLE_ST:
		stop_pwm_and_reset_states_ifnot();
		break;
	case ERROR_ST:
		stop_pwm_and_reset_states_ifnot();
		break;
	case POWER_ST:
		/* Control loop is executed here */
		control_torque();
		compute_duties();
		apply_duties();
		start_pwms_ifnot();
		break;
	}

	/* Decimation is used to reduce rate of plotting in ScopeMimicry */
	if (counter_time % decimation == 0) {
		angle_index_f = angle_index;
		Va = Vabc.a;
		duty_a = duty_abc.a;
		duty_b = duty_abc.b;
		Iq_ref = Idq_ref.q;
		Iq_meas = Idq.q;
		Vd = Vdq.d;
		Vq = Vdq.q;
		Iabc_ref = Transform::to_threephase(Idq_ref, angle_4_control);
		Ia_ref = Iabc_ref.a;
		Ib_ref = Iabc_ref.b;
		counter_time_f = (float32_t)counter_time;
		HALL1_value_f = HALL1_value;
		HALL2_value_f = HALL2_value;
		HALL3_value_f = HALL3_value;
		control_state_f = control_state;
		scope.acquire();
	}
}

int main(void)
{
	setup_routine();

	return 0;
}
