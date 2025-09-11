#ifndef DATA_OBJECTS_H
#define DATA_OBJECTS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <thingset.h>
#include <thingset/sdk.h>

/* =========================================================================
 * Configuration
 * ========================================================================= */

#ifndef POWER_NUM_LEGS
    #ifdef CONFIG_SHIELD_OWNVERTER
        #define POWER_NUM_LEGS  3   /* set to your actual number of legs */
    #else
        #define POWER_NUM_LEGS  2   /* set to your actual number of legs */
    #endif
#endif

/*
 * Subset definitions for statements and publish/subscribe
 */

/* No subsets used */
#define NO_SUBSET 0


/* =========================================================================
 * Legacy measurement symbols (addresses provided elsewhere)
 * ========================================================================= */

float32_t V1_low_value;
float32_t V2_low_value;
float32_t V_high_value;
float32_t I1_low_value;
float32_t I2_low_value;
float32_t I_high_value;

#ifdef CONFIG_SHIELD_OWNVERTER
    float32_t V3_low_value;
    float32_t I3_low_value;
    float32_t temp_value;
#else
    float32_t temp_1_value;
    float32_t temp_2_value;
#endif

float32_t duty_cycle = 0.3;
float32_t voltage_reference = 15;
float32_t voltage_setpoint = 15;

// Control references for AC inverter (grid-forming)
float32_t ctrl_vd_ref = 0.0f;   // D-axis voltage reference [V]
float32_t ctrl_vq_ref = 0.0f;   // Q-axis voltage reference [V]
float32_t ctrl_omega_ref = 314.159265f; // Angular frequency reference [rad/s] (default 50 Hz)

// Mode handling
typedef struct {
    const char *name;
    uint8_t value;
} ModeDef;

enum { MODE_IDL = 0, MODE_PWR = 1, NUM_OF_MODES = 2 };
extern const ModeDef modes[];
uint8_t mode_current = MODE_IDL;
uint8_t mode_prev;

// removed legacy xSet intermediate variables



/* Temporary storage fore measured value (ctrl task) */
float32_t meas_data;


typedef struct {
    const char *name;
    float32_t *address;      /* pointer to the live signal */
    sensor_t    channel_reference;
    float32_t   gain;        /* calibration gain */
    float32_t   offset;      /* calibration offset */
} SystemSensors;


SystemSensors system_sensors[] = {
    { "V1", &V1_low_value, V1_LOW, 1.0f, 0.0f },
    { "V2", &V2_low_value, V2_LOW, 1.0f, 0.0f },
    { "VH", &V_high_value, V_HIGH, 1.0f, 0.0f },
    { "I1", &I1_low_value, I1_LOW, 1.0f, 0.0f },
    { "I2", &I2_low_value, I2_LOW, 1.0f, 0.0f },
    { "IH", &I_high_value, I_HIGH, 1.0f, 0.0f },
#ifdef CONFIG_SHIELD_OWNVERTER
    { "V3", &V3_low_value, V3_LOW, 1.0f, 0.0f },
    { "I3", &I3_low_value, I3_LOW, 1.0f, 0.0f },
#endif
};


// Unified Function config callback
void conf_func_cb(enum thingset_callback_reason reason);

/* =========================================================================
 * Config → Function (DC/AC)
 * ========================================================================= */

// IDs for unified Function config
#define ID_CONF_FUNC            0x43
#define ID_CONF_FUNC_WDOMAIN    0x4301
#define ID_CONF_FUNC_WDC_VSCS   0x4302
#define ID_CONF_FUNC_WDC_DROOP  0x4303
#define ID_CONF_FUNC_WAC_MODE   0x4304

// Function domain and AC mode enums
typedef enum {
    FUNC_DOMAIN_DC = 0,
    FUNC_DOMAIN_AC = 1,
} func_domain_t;

typedef enum {
    FUNC_AC_GF = 0,     // Grid-forming
    FUNC_AC_GFL = 1,    // Grid-following
} func_ac_mode_t;

// Backing storage for minimal function set
uint8_t    func_domain = FUNC_DOMAIN_DC; // DC by default
bool       func_dc_vscs_enable = false;
bool       func_dc_droop_enable = false;
uint8_t    func_ac_mode = FUNC_AC_GF;




/* =========================================================================
 * Backing storage
 * ========================================================================= */


/* ---- Power / Legs ----
 */
typedef struct {
    /* Selection + reference */
    int8_t  wVar;            /* index into tracking_vars[] (0..TRACKING_VARS_COUNT-1) */
    float32_t   wRef;            /* setpoint for selected variable (REFERENCE) */
    float32_t *tracking_var;  // pointer to the live measurement
    const char *tracking_name;      // optional, for debug prints

    /* State toggles */
    bool    wLegON;            /* LEG on/off */
    bool    wCapa;           /* CAPA on/off */
    bool    wDriver;         /* DRIVER on/off (per-leg) */
    bool    wBuck;           /* BUCK mode enable */

    /* PWM-related knobs (kept in leg group for compatibility) */
    float32_t   wDuty;           /* DUTY [0..1] */
    float32_t   wPhase_deg;      /* PHASE_SHIFT [deg] */
    uint16_t wDead_rise_ns;  /* DEAD_TIME_RISING [ns] */
    uint16_t wDead_fall_ns;  /* DEAD_TIME_FALLING [ns] */
    float32_t   wFreq_Hz;        /* FREQUENCY [Hz] (may be routed to shared timer) */

} power_leg_t;

power_leg_t power_legs[POWER_NUM_LEGS] = {
    { .wLegON=false, .wCapa=false, .wDriver=false, .wBuck=false, 
      .wDuty=0.0f, .wPhase_deg=0.0f, .wDead_rise_ns=0, .wDead_fall_ns=0, 
      .wFreq_Hz=0.0f },
    { .wLegON=false, .wCapa=false, .wDriver=false, .wBuck=false, 
      .wDuty=0.0f, .wPhase_deg=0.0f, .wDead_rise_ns=0, .wDead_fall_ns=0, 
      .wFreq_Hz=0.0f },
#ifdef CONFIG_SHIELD_OWNVERTER
    { .wVar = 6, .wRef = 0.0f, .tracking_var=NULL, .tracking_name=NULL,
      .wLegON=false, .wCapa=false, .wDriver=false, .wBuck=false,
      .wDuty=0.0f, .wPhase_deg=0.0f, .wDead_rise_ns=0, .wDead_fall_ns=0, .wFreq_Hz=0.0f },
#endif

};


typedef enum {
    AC_MODE_GRID_FORMING = 0,
    AC_MODE_GRID_FOLLOWING,
    NUM_OF_AC_MODES
} ac_mode_t;

typedef enum {
    AC_PARAM_P = 0,
    AC_PARAM_Q,
    NUM_OF_AC_PARAMS
} ac_param_t;


typedef enum {
    FUNC_LEG_NONE = 0,
    FUNC_LEG_BUCK,
    FUNC_LEG_BOOST,
    FUNC_LEG_INDEPENDENT,  // nothing configured on other leg
    NUM_OF_LEG_MODES
} leg_func_mode_t;



static uint8_t ac_mode_gf = AC_MODE_GRID_FORMING;
static uint8_t ac_mode_follow = AC_MODE_GRID_FOLLOWING;
static uint8_t ac_param_p = AC_PARAM_P;
static uint8_t ac_param_q = AC_PARAM_Q;
static uint8_t received_ac_mode;
static uint8_t received_ac_param;

// Number of measurement channels defined by system_sensors[]
enum { NUM_OF_MEAS = 
#ifdef CONFIG_SHIELD_OWNVERTER
    8
#else
    6
#endif
};




/* ---- Communication (subset you requested) ---- */
static int8_t comm_sync_rRole = 1;  /* 0=Master, 1=Slave (read-only) */
static bool  comm_analog_wEnable = false;
static float32_t comm_analog_rValue  = 0.0f;


/* =========================================================================
 * ID map
 * ========================================================================= */

/* Top-level groups */
#define ID_CONF             0x4


#define ID_CONF_MODE               0x41
#define ID_CONF_MODE_MAP           0x411


#define ID_CONF_LEG_SET             0x42
#define ID_CONF_FUNC_SET            0x43
#define ID_MEAS                     0x5
#define ID_MEAS_VAL                 0x51
#define ID_MEAS_CAL                 0x52

#define ID_CONF_LEG_SET_GENERIC     0x429   // container for xSet

/* xSet function itself */
#define ID_CONF_LEG_XSET            0x4290

/* Arguments for xSet */
#define ID_CONF_LEG_SET_LEGNUM      0x4291
#define ID_CONF_LEG_SET_PARAMNUM    0x4292
#define ID_CONF_LEG_SET_VALUE       0x4293


/* Power/Legs sub-groups & items (LEG1) */
#define ID_CONF_LEG               0x420

#define ID_CONF_AC             0x46
#define ID_CONF_AC_WMODE       0x461
#define ID_CONF_AC_WPARAM      0x462

// Control subgroup for inverter references
#define ID_CONF_CTRL           0x47
#define ID_CONF_CTRL_WVD       0x4701
#define ID_CONF_CTRL_WVQ       0x4702
#define ID_CONF_CTRL_WOMEGA    0x4703

/* Measurements → individual items */
#define ID_MEAS_VAL_V1_LOW      0x512
#define ID_MEAS_VAL_V2_LOW      0x513
#define ID_MEAS_VAL_V_HIGH      0x514
#define ID_MEAS_VAL_I1_LOW      0x515
#define ID_MEAS_VAL_I2_LOW      0x516
#define ID_MEAS_VAL_I_HIGH      0x517
#define ID_MEAS_VAL_TEMP1       0x518
#define ID_MEAS_VAL_TEMP2       0x519

#ifdef CONFIG_SHIELD_OWNVERTER
    #define ID_MEAS_VAL_V3_LOW      0x51A
    #define ID_MEAS_VAL_I3_LOW      0x51B
#endif

/* /Meas calibration group */

/* Exec function */
#define ID_MEAS_XCALIB          0x521   // exec node xCalib

/* Arguments for xCalib */
#define ID_MEAS_CALIB_NUM       0x522   // wMeasNum
#define ID_MEAS_CALIB_GAIN      0x523   // wGain
#define ID_MEAS_CALIB_OFFSET    0x524   // wOffset

/* legacy measurement param map IDs removed */


/* =========================================================================
 * Groups
 * ========================================================================= */


THINGSET_ADD_GROUP(TS_ID_ROOT, ID_CONF,"Config", THINGSET_NO_CALLBACK);

/* =========================================================================
 * Debug (scope trigger)
 * ========================================================================= */

// IDs for Debug group and Scope subgroup
#define ID_DBG                  0x60
#define ID_DBG_SCOPE            0x601

// Backing storage for a write-only dump trigger
bool dbg_scope_dump = false;
// Backing storage for a manual acquisition trigger (ThingSet-controlled)
bool dbg_scope_trig = false;

// Callback to react on writes
void conf_scope_cb(enum thingset_callback_reason reason);

// Register groups and item
THINGSET_ADD_GROUP(TS_ID_ROOT, ID_DBG, "Debug", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_DBG, ID_DBG_SCOPE, "Scope", &conf_scope_cb);
THINGSET_ADD_ITEM_BOOL(ID_DBG_SCOPE, 0x6011, "wDump", &dbg_scope_dump, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_DBG_SCOPE, 0x6012, "wTrig", &dbg_scope_trig, THINGSET_ANY_RW, NO_SUBSET);

/* =========================================================================
 * Function (unified, high-level feature flags)
 * ========================================================================= */

// Top-level Function group
THINGSET_ADD_GROUP(ID_CONF, ID_CONF_FUNC, "Function", &conf_func_cb);

// Minimal initial set of parameters
THINGSET_ADD_ITEM_UINT8(ID_CONF_FUNC, ID_CONF_FUNC_WDOMAIN, "wDomain", &func_domain, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL (ID_CONF_FUNC, ID_CONF_FUNC_WDC_VSCS, "wDC_VSCS_Enable", &func_dc_vscs_enable, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL (ID_CONF_FUNC, ID_CONF_FUNC_WDC_DROOP, "wDC_Droop_Enable", &func_dc_droop_enable, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT8(ID_CONF_FUNC, ID_CONF_FUNC_WAC_MODE, "wAC_Mode", &func_ac_mode, THINGSET_ANY_RW, NO_SUBSET);

/* =========================================================================
 * Control (Vdq and frequency references)
 * ========================================================================= */
// Forward declare callback
void conf_ctrl_cb(enum thingset_callback_reason reason);
THINGSET_ADD_GROUP(ID_CONF, ID_CONF_CTRL, "Ctrl", &conf_ctrl_cb);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_CTRL, ID_CONF_CTRL_WVD,    "wVd_V",    &ctrl_vd_ref,    3, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_CTRL, ID_CONF_CTRL_WVQ,    "wVq_V",    &ctrl_vq_ref,    3, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_CTRL, ID_CONF_CTRL_WOMEGA, "wOmega_rps", &ctrl_omega_ref, 3, THINGSET_ANY_RW, NO_SUBSET);

/* =========================================================================
 * Power → Leg 1
 * ========================================================================= */

// Mode group (no exec), with writable current mode and a read-only map
void conf_mode_cb(enum thingset_callback_reason reason);
THINGSET_ADD_GROUP(ID_CONF, ID_CONF_MODE, "Mode", &conf_mode_cb);
THINGSET_ADD_ITEM_UINT8(ID_CONF_MODE, 0x4101, "wMode", &mode_current, THINGSET_ANY_RW, NO_SUBSET);
// Optional read-only map of available modes
THINGSET_ADD_GROUP(ID_CONF_MODE, ID_CONF_MODE_MAP, "Map", THINGSET_NO_CALLBACK);
static uint8_t mode_idl_const = MODE_IDL;
static uint8_t mode_pwr_const = MODE_PWR;
THINGSET_ADD_ITEM_UINT8(ID_CONF_MODE_MAP, 0x4111, "IDL", &mode_idl_const, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_UINT8(ID_CONF_MODE_MAP, 0x4112, "PWR", &mode_pwr_const, THINGSET_ANY_R, NO_SUBSET);


// Parent Leg group (container). We'll add a subgroup per leg as numeric names
THINGSET_ADD_GROUP(ID_CONF, ID_CONF_LEG, "Leg", THINGSET_NO_CALLBACK);

// Per-leg subgroups: 0..(POWER_NUM_LEGS-1)

// Callback wrappers (implemented in main.cpp)
void conf_leg_cb_0(enum thingset_callback_reason reason);
void conf_leg_cb_1(enum thingset_callback_reason reason);
void conf_leg_cb_2(enum thingset_callback_reason reason);

// IDs for subgroups
#define ID_CONF_LEG_0  0x480
#define ID_CONF_LEG_1  0x481
#define ID_CONF_LEG_2  0x482

#if (POWER_NUM_LEGS > 0)
THINGSET_ADD_GROUP(ID_CONF_LEG, ID_CONF_LEG_0, "0", &conf_leg_cb_0);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4801, "wOn",     &power_legs[0].wLegON,        THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4802, "wCapa",   &power_legs[0].wCapa,         THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4803, "wDriver", &power_legs[0].wDriver,       THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4804, "wBuck",   &power_legs[0].wBuck,         THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x4805, "wDuty",   &power_legs[0].wDuty,   3,    THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x4806, "wPhase_deg", &power_legs[0].wPhase_deg,1,THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_0,0x4807,"wDTRise_ns", &power_legs[0].wDead_rise_ns, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_0,0x4808,"wDTFall_ns", &power_legs[0].wDead_fall_ns, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x4809, "wFreq_Hz", &power_legs[0].wFreq_Hz, 1,  THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_INT8(ID_CONF_LEG_0, 0x480A, "wVar",    &power_legs[0].wVar,          THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x480B, "wRef",    &power_legs[0].wRef,    3,    THINGSET_ANY_RW, NO_SUBSET);
#endif

// (Function subgroups replaced by minimal feature set at top of file)

#if (POWER_NUM_LEGS > 1)
THINGSET_ADD_GROUP(ID_CONF_LEG, ID_CONF_LEG_1, "1", &conf_leg_cb_1);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4811, "wOn",     &power_legs[1].wLegON,        THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4812, "wCapa",   &power_legs[1].wCapa,         THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4813, "wDriver", &power_legs[1].wDriver,       THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4814, "wBuck",   &power_legs[1].wBuck,         THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x4815, "wDuty",   &power_legs[1].wDuty,   3,    THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x4816, "wPhase_deg", &power_legs[1].wPhase_deg,1,THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_1,0x4817,"wDTRise_ns", &power_legs[1].wDead_rise_ns, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_1,0x4818,"wDTFall_ns", &power_legs[1].wDead_fall_ns, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x4819, "wFreq_Hz", &power_legs[1].wFreq_Hz, 1,  THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_INT8(ID_CONF_LEG_1, 0x481A, "wVar",    &power_legs[1].wVar,          THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x481B, "wRef",    &power_legs[1].wRef,    3,    THINGSET_ANY_RW, NO_SUBSET);
#endif

#if (POWER_NUM_LEGS > 2)
THINGSET_ADD_GROUP(ID_CONF_LEG, ID_CONF_LEG_2, "2", &conf_leg_cb_2);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4821, "wOn",     &power_legs[2].wLegON,        THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4822, "wCapa",   &power_legs[2].wCapa,         THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4823, "wDriver", &power_legs[2].wDriver,       THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4824, "wBuck",   &power_legs[2].wBuck,         THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x4825, "wDuty",   &power_legs[2].wDuty,   3,    THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x4826, "wPhase_deg", &power_legs[2].wPhase_deg,1,THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_2,0x4827,"wDTRise_ns", &power_legs[2].wDead_rise_ns, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_2,0x4828,"wDTFall_ns", &power_legs[2].wDead_fall_ns, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x4829, "wFreq_Hz", &power_legs[2].wFreq_Hz, 1,  THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_INT8(ID_CONF_LEG_2, 0x482A, "wVar",    &power_legs[2].wVar,          THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x482B, "wRef",    &power_legs[2].wRef,    3,    THINGSET_ANY_RW, NO_SUBSET);
#endif






// Separate AC group removed; go through Config/Function instead



/* legacy AC ParamMap and xSet removed */




// /* =========================================================================
//  * Measurements
//  * ========================================================================= */

/* ---- Individual measurements (low-voltage rails, high-voltage, currents, temps) ---- */

THINGSET_ADD_GROUP(TS_ID_ROOT, ID_MEAS,"Measurements", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_VAL,"rValues", THINGSET_NO_CALLBACK);


THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V1_LOW, 
                        "rV1Low_V",  &V1_low_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V2_LOW, 
                        "rV2Low_V",  &V2_low_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V_HIGH,"rVHigh_V",   &V_high_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);

THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I1_LOW, "rI1Low_A",  &I1_low_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I2_LOW, "rI2Low_A",  &I2_low_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I_HIGH,"rIHigh_A",   &I_high_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);

THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_TEMP1,  "rTemp_degC",   &temp_1_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_TEMP2,  "rTemp2_degC",  &temp_2_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);

/* ---- Extra channels present on OWNVERTER builds ---- */
#ifdef CONFIG_SHIELD_OWNVERTER
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V3_LOW, "rV3Low_V",  &V3_low_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I3_LOW, "rI3Low_A",  &I3_low_value, 2,
                        THINGSET_ANY_R, NO_SUBSET);
#endif


// Per-measurement subgroups with unified callback
void meas_cb_0(enum thingset_callback_reason reason);
void meas_cb_1(enum thingset_callback_reason reason);
void meas_cb_2(enum thingset_callback_reason reason);
void meas_cb_3(enum thingset_callback_reason reason);
void meas_cb_4(enum thingset_callback_reason reason);
void meas_cb_5(enum thingset_callback_reason reason);
#ifdef CONFIG_SHIELD_OWNVERTER
void meas_cb_6(enum thingset_callback_reason reason);
void meas_cb_7(enum thingset_callback_reason reason);
#endif

// IDs for subgroups
#define ID_MEAS_CH0   0x535
#define ID_MEAS_CH1   0x536
#define ID_MEAS_CH2   0x537
#define ID_MEAS_CH3   0x538
#define ID_MEAS_CH4   0x539
#define ID_MEAS_CH5   0x53A
#ifdef CONFIG_SHIELD_OWNVERTER
#define ID_MEAS_CH6   0x53B
#define ID_MEAS_CH7   0x53C
#endif

// Channel 0 (V1)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH0, system_sensors[0].name, &meas_cb_0);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH0, 0x5351, "rValue", system_sensors[0].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH0, 0x5352, "wGain",  &system_sensors[0].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH0, 0x5353, "wOffset",&system_sensors[0].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH1, system_sensors[1].name, &meas_cb_1);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH1, 0x5361, "rValue", system_sensors[1].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH1, 0x5362, "wGain",  &system_sensors[1].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH1, 0x5363, "wOffset",&system_sensors[1].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

// Channel 2 (VH)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH2, system_sensors[2].name, &meas_cb_2);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH2, 0x5371, "rValue", system_sensors[2].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH2, 0x5372, "wGain",  &system_sensors[2].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH2, 0x5373, "wOffset",&system_sensors[2].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

// Channel 3 (I1)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH3, system_sensors[3].name, &meas_cb_3);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH3, 0x5381, "rValue", system_sensors[3].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH3, 0x5382, "wGain",  &system_sensors[3].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH3, 0x5383, "wOffset",&system_sensors[3].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

// Channel 4 (I2)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH4, system_sensors[4].name, &meas_cb_4);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH4, 0x5391, "rValue", system_sensors[4].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH4, 0x5392, "wGain",  &system_sensors[4].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH4, 0x5393, "wOffset",&system_sensors[4].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

// Channel 5 (IH)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH5, system_sensors[5].name, &meas_cb_5);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH5, 0x53A1, "rValue", system_sensors[5].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH5, 0x53A2, "wGain",  &system_sensors[5].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH5, 0x53A3, "wOffset",&system_sensors[5].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

#ifdef CONFIG_SHIELD_OWNVERTER
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH6, system_sensors[6].name, &meas_cb_6);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH6, 0x53B1, "rValue", system_sensors[6].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH6, 0x53B2, "wGain",  &system_sensors[6].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH6, 0x53B3, "wOffset",&system_sensors[6].offset, 4, THINGSET_ANY_RW, NO_SUBSET);

THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH7, system_sensors[7].name, &meas_cb_7);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH7, 0x53C1, "rValue", system_sensors[7].address, 2, THINGSET_ANY_R, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH7, 0x53C2, "wGain",  &system_sensors[7].gain, 4, THINGSET_ANY_RW, NO_SUBSET);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH7, 0x53C3, "wOffset",&system_sensors[7].offset, 4, THINGSET_ANY_RW, NO_SUBSET);
#endif

#endif /* DATA_OBJECTS_H */
