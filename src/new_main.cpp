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
    uint32_t app_task_number = task.createBackground(background_task);

    task.createCritical(critical_task, CONTROL_TASK_PERIOD_US);

    // Start tasks
    task.startBackground(app_task_number);
    task.startCritical();
}

void background_task()
{
    // Minimal app task: LED and periodic state printouts for ThingSet verification
    if (mode == MODE_IDL) {
        spin.led.turnOff();
    } else if (mode == MODE_PWR) {
        spin.led.turnOn();
    }

    static uint32_t tick = 0;
    tick++;

    // Print every ~1s to avoid flooding (given 100 ms period below)
    if ((tick % 10u) == 0u) {
        // Report high-level mode and function selection
        printk("APP mode=%u (%s) func: domain=%u dc_vscs=%u dc_droop=%u ac_mode=%u\n",
                (unsigned)mode,
                modes[(mode < NUM_OF_MODES) ? mode : 0].name,
                (unsigned)func_domain,
                (unsigned)func_dc_vscs_enable,
                (unsigned)func_dc_droop_enable,
                (unsigned)func_ac_mode);

        // Current references mapped from legs (see critical task):
        float32_t Iq_ref_dbg = (POWER_NUM_LEGS > 0) ? power_legs[0].wRef : 0.0F;
        float32_t Vpk_ref_dbg = (POWER_NUM_LEGS > 1) ? power_legs[1].wRef : 0.0F;
        printk("Setpoints: Iq_ref=%.3f A  Vpk_ref=%.3f V\n", (double)Iq_ref_dbg, (double)Vpk_ref_dbg);

        // Per-leg config snapshot
        for (uint8_t i = 0; i < POWER_NUM_LEGS; i++) {
            const char *tname = power_legs[i].tracking_name ? power_legs[i].tracking_name : "-";
            float32_t tval = (power_legs[i].tracking_var) ? *(power_legs[i].tracking_var) : 0.0F;
            printk("Leg%u: ON=%u CAPA=%u DRV=%u BUCK=%u | Duty=%.3f Ph=%.1fdeg F=%.1fHz | DT(r/f)=(%u/%u)ns | Var=%d(%s)=%.3f Ref=%.3f\n",
                    (unsigned)i,
                    (unsigned)power_legs[i].wLegON,
                    (unsigned)power_legs[i].wCapa,
                    (unsigned)power_legs[i].wDriver,
                    (unsigned)power_legs[i].wBuck,
                    (double)power_legs[i].wDuty,
                    (double)power_legs[i].wPhase_deg,
                    (double)power_legs[i].wFreq_Hz,
                    (unsigned)power_legs[i].wDead_rise_ns,
                    (unsigned)power_legs[i].wDead_fall_ns,
                    (int)power_legs[i].wVar,
                    tname,
                    (double)tval,
                    (double)power_legs[i].wRef);
        }

        // Key measurements for quick sanity (if available)
        printk("Meas: V1=%.3f V  I1=%.3f A  V2=%.3f V  I2=%.3f A  VH=%.3f V  IH=%.3f A\n",
                (double)V1_low_value, (double)I1_low_value,
                (double)V2_low_value, (double)I2_low_value,
                (double)V_high_value, (double)I_high_value);
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
}

int main(void)
{
    setup_routine();
    return 0;
}

#endif // USE_NEW_MAIN
