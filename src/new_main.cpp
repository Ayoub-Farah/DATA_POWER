/*
 * Alternative fused main with Grid-Forming control integrated with ThingSet.
 * Enable by defining USE_NEW_MAIN in your build flags to replace src/main.cpp.
 */

#ifdef USE_NEW_MAIN

#include <zephyr/console/console.h>

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

    // Initialize single phase inverter in Grid-Forming mode
    // V_bus initial guess 60 V; will be updated from measurement each cycle
    // grid_Vpk 10 V, grid_w0 for 50 Hz
    const float32_t Vbus_init = 60.0F;
    const float32_t grid_Vpk  = 10.0F;
    const float32_t grid_w0   = 2.0F * PI * 50.0F;
    inverter.init(FORMING, Vbus_init, grid_Vpk, grid_w0, Ts);

    // Create tasks
    uint32_t app_task_number = task.createBackground([](){
        // Minimal app task: LED and optional prints
        if (mode == MODE_IDL) {
            spin.led.turnOff();
        } else if (mode == MODE_PWR) {
            spin.led.turnOn();
        }
        task.suspendBackgroundMs(100);
    });

    task.createCritical([](){
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

        // MODE_PWR: Grid-forming control

        // ThingSet-controlled references mapping:
        //  - Reactive current reference from Leg0.wRef (A)
        //  - Voltage amplitude from Leg1.wRef (Vpk)
        const float32_t Iq_ref = clampf(power_legs[0].wRef, -3.0F, 3.0F);
        const float32_t Vpk_ref = clampf(power_legs[1 % POWER_NUM_LEGS].wRef, 1.0F, 30.0F);

        dqo_t Vdq_ref = { .d = Vpk_ref, .q = 0.0F, .o = 0.0F };
        dqo_t Idq_ref = { .d = 0.0F,    .q = Iq_ref, .o = 0.0F };
        inverter.setVdqRef(Vdq_ref);
        inverter.setIdqRef(Idq_ref);

        // Compute duty from measured grid voltage/current
        float32_t duty = inverter.calculateDuty(V1_low_value, I1_low_value);
        duty = clampf(duty, 0.0F, 1.0F);

        shield.power.setDutyCycle(ALL, duty);

        if (!pwm_enable) {
            pwm_enable = true;
            shield.power.start(ALL);
        }
    }, CONTROL_TASK_PERIOD_US);

    // Start tasks
    task.startBackground(app_task_number);
    task.startCritical();
}

int main(void)
{
    setup_routine();
    return 0;
}

#endif // USE_NEW_MAIN
