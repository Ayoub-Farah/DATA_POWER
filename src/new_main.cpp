/*
 * Alternative fused main with Grid-Forming control integrated with ThingSet.
 * Enable by defining USE_NEW_MAIN in your build flags to replace src/main.cpp.
 */

#ifdef USE_NEW_MAIN

#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>

#include "SpinAPI.h"
#include "ShieldAPI.h"
#include "TaskAPI.h"

#include "pid.h"

#include "singlePhaseInverter.h"
#include "trigo.h"

#include "user_data_api.h"

// Runtime app mode (used by ThingSet callbacks)
uint8_t mode = MODE_IDL;

// Available modes (declared extern in user_data_objects.h)
const ModeDef modes[] = {
    { "IDL", MODE_IDL },
    { "PWR", MODE_PWR },
};

// Control task period [us]
static constexpr uint32_t CONTROL_TASK_PERIOD_US = 100;
static float32_t Ts = CONTROL_TASK_PERIOD_US * 1.0e-6F;

// Inverter controller
static singlePhaseInverter inverter;

// PWM enable state
static bool pwm_enable = false;

// Simple clamps
static inline float32_t clampf(float32_t v, float32_t lo, float32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void critical_task(void);
void background_task(void);
void app_apply_ac_mode(uint8_t new_ac_mode);

/*--------------SETUP FUNCTIONS------------------------------- */
static void setup_routine()
{
    // Initialize power stage (Buck on all legs, update as needed for your hardware)
    shield.power.initBuck(ALL);

    // Sensors
    shield.sensors.enableDefaultTwistSensors();

    // Load stored calibration (gain/offset) from NVM into runtime map
    for (int i = 0; i < NUM_OF_MEAS; i++) {
        float32_t g = shield.sensors.retrieveStoredParameterValue(system_sensors[i].channel_reference, gain);
        float32_t o = shield.sensors.retrieveStoredParameterValue(system_sensors[i].channel_reference, offset);
        system_sensors[i].gain = g;
        system_sensors[i].offset = o;
    }

    // Initialize single phase inverter according to configured AC mode
    // V_bus initial guess 60 V; will be updated from measurement each cycle
    // grid_Vpk 10 V, grid_w0 for 50 Hz
    const float32_t Vbus_init = 60.0F;
    const float32_t grid_Vpk  = 10.0F;
    const float32_t grid_w0   = 2.0F * PI * 50.0F;
    inverter.init((func_ac_mode == FUNC_AC_GF) ? FORMING : FOLLOWING,
                  Vbus_init, grid_Vpk, grid_w0, Ts);
    // Apply initial mode gating/sync behavior
    app_apply_ac_mode(func_ac_mode);

    // Create tasks
    uint32_t app_task_number = task.createBackground(background_task);

    task.createCritical(critical_task, CONTROL_TASK_PERIOD_US);

    // Start tasks
    task.startBackground(app_task_number);
    task.startCritical();
}

void background_task()
{
    // Compile-time switch to keep strings out of binary when disabled
    #ifndef ENABLE_VERBOSE_PRINTS
    #define ENABLE_VERBOSE_PRINTS 0
    #endif
    // Minimal app task: LED and periodic state printouts for ThingSet verification
    if (mode == MODE_IDL) {
        spin.led.turnOff();
    } else if (mode == MODE_PWR) {
        spin.led.turnOn();
    }

    static uint32_t tick = 0;
    tick++;

    // Simple test when no DC bus present: reflect AC role via leg state
    // - GF: turn leg 0 ON
    // - GFL: blink leg 0 ON/OFF
    if (mode == MODE_PWR && V_high_value < 5.0f && POWER_NUM_LEGS > 0) {
        if (func_ac_mode == FUNC_AC_GF) {
            power_legs[0].wLegON = true;
        } else {
            // blink every 500 ms (5 x 100ms ticks)
            if ((tick % 5u) == 0u) {
                power_legs[0].wLegON = !power_legs[0].wLegON;
            }
        }
    }

    // Print every ~1s to avoid flooding (given 100 ms period below)
    if ((tick % 10u) == 0u) {
        printk("APP m=%u ac=%u\n", (unsigned)mode, (unsigned)func_ac_mode);
#if ENABLE_VERBOSE_PRINTS
        // Optional diagnostics (disabled by default to save flash)
        float32_t Iq_ref_dbg = (POWER_NUM_LEGS > 0) ? power_legs[0].wRef : 0.0F;
        float32_t Vpk_ref_dbg = (POWER_NUM_LEGS > 1) ? power_legs[1].wRef : 0.0F;
        printk("Setpoints: Iq=%.3f Vpk=%.3f\n", (double)Iq_ref_dbg, (double)Vpk_ref_dbg);
        for (uint8_t i = 0; i < POWER_NUM_LEGS; i++) {
            printk("Leg%u: ON=%u DRV=%u\n", (unsigned)i,
                   (unsigned)power_legs[i].wLegON,
                   (unsigned)power_legs[i].wDriver);
        }
        printk("Meas: V1=%.2f VH=%.2f\n", (double)V1_low_value, (double)V_high_value);
#endif
    }

    task.suspendBackgroundMs(100);
}


void critical_task()
{
    // Acquire latest measurements
    float32_t val;

    val = shield.sensors.getLatestValue(I1_LOW);
    if (val != NO_VALUE) I1_low_value = val;

    val = shield.sensors.getLatestValue(V1_LOW);
    if (val != NO_VALUE) V1_low_value = val;

    val = shield.sensors.getLatestValue(V2_LOW);
    if (val != NO_VALUE) V2_low_value = val;

    val = shield.sensors.getLatestValue(I2_LOW);
    if (val != NO_VALUE) I2_low_value = val;

    val = shield.sensors.getLatestValue(I_HIGH);
    if (val != NO_VALUE) I_high_value = val;

    val = shield.sensors.getLatestValue(V_HIGH);
    if (val != NO_VALUE) V_high_value = val;

    // Update inverter DC bus estimate from measurement if valid
    if (V_high_value > 1.0F) {
        inverter.setVBus(V_high_value);
    }

    if (mode == MODE_IDL) {
        if (pwm_enable) {
            shield.power.stop(ALL);
            pwm_enable = false;
        }
        return;

    }

    // MODE_PWR
    // Guard: only attempt PWM if DC bus is present
    const bool bus_ok = (V_high_value > 10.0f);
    if (!bus_ok) {
        if (pwm_enable) {
            shield.power.stop(ALL);
            pwm_enable = false;
        }
        return;
    }

    // Compute duty from inverter
    float32_t v_meas = V1_low_value;
    float32_t i_meas = I1_low_value;
    float32_t duty = inverter.calculateDuty(v_meas, i_meas);
    duty = clampf(duty, 0.0f, 0.95f);
    shield.power.setDutyCycle(ALL, duty);

    // Gating rules
    if (func_ac_mode == FUNC_AC_GF) {
        inverter.setPowerOn(true);
        if (!pwm_enable) {
            pwm_enable = true;
            shield.power.start(ALL);
        }
    } else {
        // FOLLOWING: keep power off until sync is achieved
        if (inverter.getSync()) {
            inverter.setPowerOn(true);
            if (!pwm_enable) {
                pwm_enable = true;
                shield.power.start(ALL);
            }
        } else {
            inverter.setPowerOn(false);
            if (pwm_enable) {
                shield.power.stop(ALL);
                pwm_enable = false;
            }
        }
    }
}

int main(void)
{
    setup_routine();
    return 0;
}

// Apply AC mode changes (from ThingSet callback)
void app_apply_ac_mode(uint8_t new_ac_mode)
{
    if (new_ac_mode == FUNC_AC_GF) {
        inverter.setMode(FORMING);
        inverter.setSyncOff();
        inverter.setPowerOn(true);
        // Do not start PWM here; critical task will start it when bus is OK
    } else {
        inverter.setMode(FOLLOWING);
        inverter.setSyncOff();
        inverter.setPowerOn(false);
        // Ensure PWM is stopped until sync
        if (pwm_enable) {
            shield.power.stop(ALL);
            pwm_enable = false;
        }
    }
}

#endif // USE_NEW_MAIN
