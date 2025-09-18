/*
 * DC-only main: voltage/current mode control with signed references via ThingSet.
 * Enable by defining USE_DC_MAIN in your build flags to replace src/main.cpp.
 */

#ifdef USE_DC_MAIN

#include <zephyr/console/console.h>
#include <zephyr/sys/printk.h>

#include "SpinAPI.h"
#include "ShieldAPI.h"
#include "TaskAPI.h"

#include "pid.h"

#include "user_data_api.h"

// Runtime app mode (IDL/PWR) used by ThingSet callbacks
uint8_t mode = MODE_IDL;

// Available modes (declared extern in user_data_objects.h)
const ModeDef modes[] = {
    { "IDL", MODE_IDL },
    { "PWR", MODE_PWR },
};

// Control task period [us]
static constexpr uint32_t CONTROL_TASK_PERIOD_US = 100; // 10 kHz
static float32_t Ts = CONTROL_TASK_PERIOD_US * 1.0e-6F;

// PID controller for DC voltage/current loop.
// We center the controller around 0.5 duty by using symmetric output bounds.
static float32_t kp = 0.000215f;
static float32_t Ti = 7.5175e-5f;
static float32_t Td = 0.0f;
static float32_t N  = 0.0f;
// Controller output range (added to 0.5 to get duty)
static float32_t ctrl_hi = 0.45f;
static float32_t ctrl_lo = -0.45f;
static PidParams pid_params(Ts, kp, Ti, Td, N, ctrl_lo, ctrl_hi);
static Pid pid_legs[POWER_NUM_LEGS];

// PWM enable state
// Per-leg PWM enable flags
static bool pwm_enable[POWER_NUM_LEGS] = {false};

// Forward declarations
static void setup_routine(void);
static void background_task(void);
static void critical_task(void);

// Simple helpers
static inline float32_t clampf(float32_t v, float32_t lo, float32_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
// No polarity or mode reconfig required in this DC app

static void setup_routine()
{
    // Configure all legs as Buck in voltage mode by default
    shield.power.initBuck(ALL, VOLTAGE_MODE);

    // Sensors
    shield.sensors.enableDefaultTwistSensors();
    // Load stored calibration (gain/offset) from NVM
    for (int i = 0; i < NUM_OF_MEAS; i++) {
        float32_t g = shield.sensors.retrieveStoredParameterValue(system_sensors[i].channel_reference, gain);
        float32_t o = shield.sensors.retrieveStoredParameterValue(system_sensors[i].channel_reference, offset);
        system_sensors[i].gain = g;
        system_sensors[i].offset = o;
    }

    // Initialize controller
    for (uint8_t i = 0; i < POWER_NUM_LEGS; i++) {
        pid_legs[i].init(pid_params);
    }

    // Default DC modes and refs are already zero-initialized

    // Tasks
    uint32_t app_task_number = task.createBackground(background_task);
    task.createCritical(critical_task, CONTROL_TASK_PERIOD_US);
    task.startBackground(app_task_number);
    task.startCritical();
}

static void background_task()
{
    if (mode == MODE_IDL) {
        spin.led.turnOff();
    } else if (mode == MODE_PWR) {
        spin.led.turnOn();
    }

    static uint32_t tick = 0;
    tick++;
    if ((tick % 10u) == 0u) { // ~1 s
        printk("DC m=%u | L0 V=%.2f I=%.2f Ref=%.2f M=%u | L1 V=%.2f I=%.2f Ref=%.2f M=%u\n",
               (unsigned)mode,
               (double)V1_low_value, (double)I1_low_value,
               (double)power_legs[0].wRef, (unsigned)power_legs[0].wModeVC,
               (double)V2_low_value, (double)I2_low_value,
               (double)power_legs[1].wRef, (unsigned)power_legs[1].wModeVC);
    }
    task.suspendBackgroundMs(100);
}

static void critical_task()
{
    // Update measurements
    float32_t val;
    val = shield.sensors.getLatestValue(I1_LOW); if (val != NO_VALUE) I1_low_value = val;
    val = shield.sensors.getLatestValue(V1_LOW); if (val != NO_VALUE) V1_low_value = val;
    val = shield.sensors.getLatestValue(V2_LOW); if (val != NO_VALUE) V2_low_value = val;
    val = shield.sensors.getLatestValue(I2_LOW); if (val != NO_VALUE) I2_low_value = val;
    val = shield.sensors.getLatestValue(I_HIGH); if (val != NO_VALUE) I_high_value = val;
    val = shield.sensors.getLatestValue(V_HIGH); if (val != NO_VALUE) V_high_value = val;

    // Mode handling
    if (mode == MODE_IDL) {
        shield.power.stop(ALL);
        for (uint8_t i = 0; i < POWER_NUM_LEGS; i++) pwm_enable[i] = false;
        return;
    }

    // MODE_PWR
    if (V_high_value < 5.0f) {
        shield.power.stop(ALL);
        for (uint8_t i = 0; i < POWER_NUM_LEGS; i++) pwm_enable[i] = false;
        return;
    }

    // Per-leg control using per-leg mode selection (voltage/current) and reference (wRef)
    static uint8_t prev_mode_vc[POWER_NUM_LEGS] = {0xFF};
    for (uint8_t i = 0; i < POWER_NUM_LEGS; i++) {
        // Reset PID on mode change
        if (prev_mode_vc[i] != power_legs[i].wModeVC) {
            pid_legs[i].init(pid_params);
            prev_mode_vc[i] = power_legs[i].wModeVC;
        }
        // Select measurement based on mode and leg index
        float32_t meas = 0.0f;
        switch (power_legs[i].wModeVC) {
            case DC_MODE_VOLTAGE:
                meas = (i == 0) ? V1_low_value : V2_low_value;
                break;
            case DC_MODE_CURRENT:
                meas = (i == 0) ? I1_low_value : I2_low_value;
                break;
            default:
                meas = (i == 0) ? V1_low_value : V2_low_value;
                break;
        }
        float32_t ref = power_legs[i].wRef;

        float32_t duty = pid_legs[i].calculateWithReturn(ref, meas); // controller output
        // duty = clampf(duty, 0.05f, 0.95f);

        shield.power.setDutyCycle(static_cast<leg_t>(i), duty);
        if (power_legs[i].wLegON) {
            if (!pwm_enable[i]) {
                pwm_enable[i] = true;
                shield.power.start(static_cast<leg_t>(i));
            }
        } else {
            if (pwm_enable[i]) {
                shield.power.stop(static_cast<leg_t>(i));
                pwm_enable[i] = false;
            }
        }
    }
}

int main(void)
{
    setup_routine();
    return 0;
}

void app_apply_dc_mode_for_leg(uint8_t leg, uint8_t new_dc_mode)
{
    // No hardware reconfiguration; selection only affects which variable the PID uses
    ARG_UNUSED(leg);
    ARG_UNUSED(new_dc_mode);
}

// Apply DC mode changes (ThingSet callback)
void app_apply_dc_mode(uint8_t new_dc_mode)
{
    // Legacy: apply to leg 0
    app_apply_dc_mode_for_leg(0, new_dc_mode);
}


// ===== Stubs for AC/inverter-oriented callbacks referenced by ThingSet =====
void app_apply_ac_mode(uint8_t new_ac_mode)
{
    ARG_UNUSED(new_ac_mode);
    printk("AC mode change ignored in DC main.\n");
}

void app_dump_scope(void)
{
    printk("Scope dump not available in DC main.\n");
}

void app_set_scope_trigger(bool trig)
{
    ARG_UNUSED(trig);
}

void app_apply_ctrl_refs(float32_t vd, float32_t vq, float32_t omega)
{
    ARG_UNUSED(vd);
    ARG_UNUSED(vq);
    ARG_UNUSED(omega);
}

#endif // USE_DC_MAIN
