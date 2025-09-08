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

/* UART serial */
#define SUBSET_SER  (1U << 0)
/* CAN bus */
#define SUBSET_CAN  (1U << 1)
/* Control data sent and received via CAN */
#define SUBSET_CTRL (1U << 3)


/* =========================================================================
 * Legacy measurement symbols (addresses provided elsewhere)
 * ========================================================================= */

static float32_t V1_low_value;
static float32_t V2_low_value;
static float32_t V_high_value;
static float32_t I1_low_value;
static float32_t I2_low_value;
static float32_t I_high_value;

#ifdef CONFIG_SHIELD_OWNVERTER
    static float32_t V3_low_value;
    static float32_t I3_low_value;
    static float32_t temp_value;
#else
    static float32_t temp_1_value;
    static float32_t temp_2_value;
#endif

static float32_t duty_cycle = 0.3;
static float32_t voltage_reference = 15;
static float32_t voltage_setpoint = 15;

extern uint8_t idle_mode;
extern uint8_t power_mode;
static uint8_t local_mode;

static uint8_t received_leg_number;
static uint8_t received_param_number;
static float32_t received_value;
static bool leg_set_value;


static uint8_t received_meas_number;
static float32_t received_meas_gain;
static float32_t received_meas_offset;


static uint8_t received_leg_func_num;



/* Temporary storage fore measured value (ctrl task) */
static float32_t meas_data;


typedef struct {
    const char *name;
    float32_t *address;      /* pointer to the live signal */
    sensor_t    channel_reference;
    float32_t   gain;        /* calibration gain */
    float32_t   offset;      /* calibration offset */
} SystemSensors;


static SystemSensors system_sensors[] = {
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


void conf_set_mode(void);
void conf_set_leg_on(void);
void conf_set_leg(void);
void meas_set_calib(void);
void conf_set_ac_mode(void);
void conf_set_ac_param(void);

// ThingSet group callback for /Config/AC
void conf_set_ac(enum thingset_callback_reason reason);

// ThingSet group callback for /Config/Leg (map-style writes)
void conf_set_legs(enum thingset_callback_reason reason);




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

static power_leg_t power_legs[POWER_NUM_LEGS] = {
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

typedef enum {
    MEAS_V1 = 0,
    MEAS_V2,
    MEAS_VH,
    MEAS_I1,
    MEAS_I2,
    MEAS_IH,
#ifdef CONFIG_SHIELD_OWNVERTER
    MEAS_V3,
    MEAS_I3,
#endif
    NUM_OF_MEAS
} meas_param_t;

// Items inside ParamMap = index → name
static uint8_t meas_v1 = MEAS_V1;
static uint8_t meas_v2 = MEAS_V2;
static uint8_t meas_vh = MEAS_VH;
static uint8_t meas_i1 = MEAS_I1;
static uint8_t meas_i2 = MEAS_I2;
static uint8_t meas_ih = MEAS_IH;
#ifdef CONFIG_SHIELD_OWNVERTER
static uint8_t meas_v3 = MEAS_V3;
static uint8_t meas_i3 = MEAS_I3;
#endif

typedef enum {
    PARAM_LEG_ON = 0,
    PARAM_CAPA,
    PARAM_DRIVER,
    PARAM_BUCK,
    PARAM_BOOST,
    PARAM_DUTY,
    PARAM_PHASE,
    PARAM_DTRISE,
    PARAM_DTFALL,
    PARAM_FREQ,
    PARAM_VAR,
    PARAM_REF,
    NUM_OF_PARAMS  // always last = total number
} leg_param_t;

static uint8_t param_on     = PARAM_LEG_ON;
static uint8_t param_capa   = PARAM_CAPA;
static uint8_t param_drv    = PARAM_DRIVER;
static uint8_t param_buck   = PARAM_BUCK;
static uint8_t param_duty   = PARAM_DUTY;
static uint8_t param_phase  = PARAM_PHASE;
static uint8_t param_dtrise = PARAM_DTRISE;
static uint8_t param_dtfall = PARAM_DTFALL;
static uint8_t param_freq   = PARAM_FREQ;
static uint8_t param_var   = PARAM_VAR;
static uint8_t param_ref   = PARAM_REF;




void *VariableMap[POWER_NUM_LEGS][NUM_OF_PARAMS] = {
    [0] = {
        &power_legs[0].wLegON,
        &power_legs[0].wCapa,
        &power_legs[0].wDriver,
        &power_legs[0].wBuck,
        &power_legs[0].wDuty,
        &power_legs[0].wPhase_deg,
        &power_legs[0].wDead_rise_ns,
        &power_legs[0].wDead_fall_ns,
        &power_legs[0].wFreq_Hz,
        &power_legs[0].wVar,
        &power_legs[0].wRef
    },
    [1] = {
        &power_legs[1].wLegON,
        &power_legs[1].wCapa,
        &power_legs[1].wDriver,
        &power_legs[1].wBuck,
        &power_legs[1].wDuty,
        &power_legs[1].wPhase_deg,
        &power_legs[1].wDead_rise_ns,
        &power_legs[1].wDead_fall_ns,
        &power_legs[1].wFreq_Hz,
        &power_legs[1].wVar,
        &power_legs[1].wRef
    },
#ifdef CONFIG_SHIELD_OWNVERTER
    [2] = {
        &power_legs[2].wLegON,
        &power_legs[2].wCapa,
        &power_legs[2].wDriver,
        &power_legs[2].wBuck,
        &power_legs[2].wDuty,
        &power_legs[2].wPhase_deg,
        &power_legs[2].wDead_rise_ns,
        &power_legs[2].wDead_fall_ns,
        &power_legs[2].wFreq_Hz,
        &power_legs[2].wVar,
        &power_legs[2].wRef
    }
#endif
};


/* ---- Communication (subset you requested) ---- */
static int8_t comm_sync_rRole = 1;  /* 0=Master, 1=Slave (read-only) */
static bool  comm_analog_wEnable = false;
static float32_t comm_analog_rValue  = 0.0f;

/* =========================================================================
 * Empty exec functions (placeholders)
 * ========================================================================= */

static void Measurements_xCalibrate(void) { /* empty by request */ }

/* =========================================================================
 * ID map
 * ========================================================================= */

/* Top-level groups */
#define ID_CONF             0x4


#define ID_CONF_MOD_SET            0x41
#define ID_CONF_MOD_SET_INPUT      0x411
#define ID_CONF_MOD_GET            0x412
#define ID_CONF_MOD_TYPE_0         0x4120
#define ID_CONF_MOD_TYPE_1         0x4121
#define ID_CONF_MOD_TYPE_2         0x4122
#define ID_CONF_MOD_TYPE_3         0x4123
#define ID_CONF_MOD_TYPE_4         0x4124
#define ID_CONF_MOD_TYPE_5         0x4125
#define ID_CONF_MOD_TYPE_6         0x4126


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
// #define ID_CONF_LEG_GET_VAL    0x423
// #define ID_CONF_LEG_GET_NAME   0x424


// #define ID_CONF_LEG_SET_VAR    0x423
// #define ID_CONF_LEG_SET_REF    0x424


/* legacy /Config/Leg ParamMap group and items removed */


/* legacy xSet items removed */

/* legacy function map removed */


#define ID_CONF_AC             0x46
#define ID_CONF_AC_WMODE       0x461
#define ID_CONF_AC_WPARAM      0x462

// #define ID_CONF_AC_PARAMMAP    0x460
// #define ID_CONF_AC_MODE_GF     0x4601
// #define ID_CONF_AC_MODE_FOLLOW 0x4602

// #define ID_CONF_AC_PARAMMAP2   0x461
// #define ID_CONF_AC_PARAM_P     0x4611
// #define ID_CONF_AC_PARAM_Q     0x4612

// #define ID_CONF_AC_XSETMODE    0x462

// #define ID_CONF_AC_XSETPARAM   0x463


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
#define ID_MEAS_CALIB           0x520   // group "Calib"

/* Exec function */
#define ID_MEAS_XCALIB          0x521   // exec node xCalib

/* Arguments for xCalib */
#define ID_MEAS_CALIB_NUM       0x522   // wMeasNum
#define ID_MEAS_CALIB_GAIN      0x523   // wGain
#define ID_MEAS_CALIB_OFFSET    0x524   // wOffset

#define ID_MEAS_PARAMMAP        0x510

#define ID_MEAS_PARAM_V1        0x5101
#define ID_MEAS_PARAM_V2        0x5102
#define ID_MEAS_PARAM_VH        0x5103
#define ID_MEAS_PARAM_I1        0x5104
#define ID_MEAS_PARAM_I2        0x5105
#define ID_MEAS_PARAM_IH        0x5106
#ifdef CONFIG_SHIELD_OWNVERTER
    #define ID_MEAS_PARAM_V3    0x5107
    #define ID_MEAS_PARAM_I3    0x5108
#endif


/* =========================================================================
 * Groups
 * ========================================================================= */


THINGSET_ADD_GROUP(TS_ID_ROOT, ID_CONF,"Config", THINGSET_NO_CALLBACK);
// THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CAL,"Cal",          THINGSET_NO_CALLBACK);
// THINGSET_ADD_GROUP(TS_ID_ROOT, ID_COMM,"Communication", THINGSET_NO_CALLBACK);
// THINGSET_ADD_GROUP(ID_COMM, ID_COMM_SYNC,"Sync",         THINGSET_NO_CALLBACK);
// THINGSET_ADD_GROUP(ID_COMM, ID_COMM_ANALOG,"Analog",       THINGSET_NO_CALLBACK);

/* =========================================================================
 * Power → Leg 1
 * ========================================================================= */

THINGSET_ADD_FN_VOID(ID_CONF,ID_CONF_MOD_SET, "xMode", &conf_set_mode, THINGSET_ANY_RW);
THINGSET_ADD_ITEM_UINT8(ID_CONF_MOD_SET,ID_CONF_MOD_SET_INPUT, "wMode", &local_mode,  THINGSET_ANY_RW, SUBSET_SER);

THINGSET_ADD_GROUP(ID_CONF, ID_CONF_MOD_GET,"xGetMode", THINGSET_NO_CALLBACK);
THINGSET_ADD_ITEM_UINT8(ID_CONF_MOD_GET, ID_CONF_MOD_TYPE_0, "IDLEMODE", &idle_mode,  THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_CONF_MOD_GET, ID_CONF_MOD_TYPE_1, "POWERMODE", &power_mode, THINGSET_ANY_R, SUBSET_SER);


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
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4801, "wOn",     &power_legs[0].wLegON,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4802, "wCapa",   &power_legs[0].wCapa,         THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4803, "wDriver", &power_legs[0].wDriver,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_0, 0x4804, "wBuck",   &power_legs[0].wBuck,         THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x4805, "wDuty",   &power_legs[0].wDuty,   3,    THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x4806, "wPhase_deg", &power_legs[0].wPhase_deg,1,THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_0,0x4807,"wDTRise_ns", &power_legs[0].wDead_rise_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_0,0x4808,"wDTFall_ns", &power_legs[0].wDead_fall_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x4809, "wFreq_Hz", &power_legs[0].wFreq_Hz, 1,  THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_INT8(ID_CONF_LEG_0, 0x480A, "wVar",    &power_legs[0].wVar,          THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_0,0x480B, "wRef",    &power_legs[0].wRef,    3,    THINGSET_ANY_RW, SUBSET_SER);
#endif

#if (POWER_NUM_LEGS > 1)
THINGSET_ADD_GROUP(ID_CONF_LEG, ID_CONF_LEG_1, "1", &conf_leg_cb_1);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4811, "wOn",     &power_legs[1].wLegON,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4812, "wCapa",   &power_legs[1].wCapa,         THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4813, "wDriver", &power_legs[1].wDriver,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_1, 0x4814, "wBuck",   &power_legs[1].wBuck,         THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x4815, "wDuty",   &power_legs[1].wDuty,   3,    THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x4816, "wPhase_deg", &power_legs[1].wPhase_deg,1,THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_1,0x4817,"wDTRise_ns", &power_legs[1].wDead_rise_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_1,0x4818,"wDTFall_ns", &power_legs[1].wDead_fall_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x4819, "wFreq_Hz", &power_legs[1].wFreq_Hz, 1,  THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_INT8(ID_CONF_LEG_1, 0x481A, "wVar",    &power_legs[1].wVar,          THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_1,0x481B, "wRef",    &power_legs[1].wRef,    3,    THINGSET_ANY_RW, SUBSET_SER);
#endif

#if (POWER_NUM_LEGS > 2)
THINGSET_ADD_GROUP(ID_CONF_LEG, ID_CONF_LEG_2, "2", &conf_leg_cb_2);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4821, "wOn",     &power_legs[2].wLegON,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4822, "wCapa",   &power_legs[2].wCapa,         THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4823, "wDriver", &power_legs[2].wDriver,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL(ID_CONF_LEG_2, 0x4824, "wBuck",   &power_legs[2].wBuck,         THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x4825, "wDuty",   &power_legs[2].wDuty,   3,    THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x4826, "wPhase_deg", &power_legs[2].wPhase_deg,1,THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_2,0x4827,"wDTRise_ns", &power_legs[2].wDead_rise_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_CONF_LEG_2,0x4828,"wDTFall_ns", &power_legs[2].wDead_fall_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x4829, "wFreq_Hz", &power_legs[2].wFreq_Hz, 1,  THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_INT8(ID_CONF_LEG_2, 0x482A, "wVar",    &power_legs[2].wVar,          THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_CONF_LEG_2,0x482B, "wRef",    &power_legs[2].wRef,    3,    THINGSET_ANY_RW, SUBSET_SER);
#endif






// Group for AC
// THINGSET_ADD_GROUP(ID_CONF, ID_CONF_AC, "AC", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_CONF, ID_CONF_AC, "AC", &conf_set_ac);

// Variables to hold configuration
static uint8_t ac_mode;
static uint8_t ac_param;

// Shadow copies to detect which fields changed across a write
static uint8_t ac_mode_prev;
static uint8_t ac_param_prev;

THINGSET_ADD_ITEM_UINT8(ID_CONF_AC, ID_CONF_AC_WMODE,
                        "wMode", &ac_mode, THINGSET_ANY_RW, SUBSET_SER);

THINGSET_ADD_ITEM_UINT8(ID_CONF_AC, ID_CONF_AC_WPARAM,
                        "wParam", &ac_param, THINGSET_ANY_RW, SUBSET_SER);



// // ParamMap: AC modes
// THINGSET_ADD_GROUP(ID_CONF_AC, ID_CONF_AC_PARAMMAP, "ModeMap", THINGSET_NO_CALLBACK);
// THINGSET_ADD_ITEM_UINT8(ID_CONF_AC_PARAMMAP, ID_CONF_AC_MODE_GF, "GRID_FORMING", &ac_mode_gf, THINGSET_ANY_R, SUBSET_SER);
// THINGSET_ADD_ITEM_UINT8(ID_CONF_AC_PARAMMAP, ID_CONF_AC_MODE_FOLLOW, "GRID_FOLLOWING", &ac_mode_follow, THINGSET_ANY_R, SUBSET_SER);

// // ParamMap: AC parameters
// THINGSET_ADD_GROUP(ID_CONF_AC, ID_CONF_AC_PARAMMAP2, "ParamMap", THINGSET_NO_CALLBACK);
// THINGSET_ADD_ITEM_UINT8(ID_CONF_AC_PARAMMAP2, ID_CONF_AC_PARAM_P, "P", &ac_param_p, THINGSET_ANY_R, SUBSET_SER);
// THINGSET_ADD_ITEM_UINT8(ID_CONF_AC_PARAMMAP2, ID_CONF_AC_PARAM_Q, "Q", &ac_param_q, THINGSET_ANY_R, SUBSET_SER);

// // Exec: set AC mode
// THINGSET_ADD_FN_VOID(ID_CONF_AC, ID_CONF_AC_XSETMODE, "xSetMode", &conf_set_ac_mode, THINGSET_ANY_RW);
// THINGSET_ADD_ITEM_UINT8(ID_CONF_AC_XSETMODE, ID_CONF_AC_WMODE, "wMode", &received_ac_mode, THINGSET_ANY_RW, SUBSET_SER);

// // Exec: set AC control parameter
// THINGSET_ADD_FN_VOID(ID_CONF_AC, ID_CONF_AC_XSETPARAM, "xSetParam", &conf_set_ac_param, THINGSET_ANY_RW);
// THINGSET_ADD_ITEM_UINT8(ID_CONF_AC_XSETPARAM, ID_CONF_AC_WPARAM, "wParam", &received_ac_param, THINGSET_ANY_RW, SUBSET_SER);




// /* =========================================================================
//  * Measurements
//  * ========================================================================= */

/* ---- Individual measurements (low-voltage rails, high-voltage, currents, temps) ---- */

THINGSET_ADD_GROUP(TS_ID_ROOT, ID_MEAS,"Measurements", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_VAL,"rValues", THINGSET_NO_CALLBACK);


THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V1_LOW, 
                        "rV1Low_V",  &V1_low_value, 2,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V2_LOW, 
                        "rV2Low_V",  &V2_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V_HIGH,"rVHigh_V",   &V_high_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);

THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I1_LOW, "rI1Low_A",  &I1_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I2_LOW, "rI2Low_A",  &I2_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I_HIGH,"rIHigh_A",   &I_high_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);

THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_TEMP1,  "rTemp_degC",   &temp_1_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_TEMP2,  "rTemp2_degC",  &temp_2_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);

/* ---- Extra channels present on OWNVERTER builds ---- */
#ifdef CONFIG_SHIELD_OWNVERTER
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_V3_LOW, "rV3Low_V",  &V3_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_VAL, ID_MEAS_VAL_I3_LOW, "rI3Low_A",  &I3_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
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
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH0, 0x5351, "rValue", system_sensors[0].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH0, 0x5352, "wGain",  &system_sensors[0].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH0, 0x5353, "wOffset",&system_sensors[0].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

// Channel 1 (V2)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH1, system_sensors[1].name, &meas_cb_1);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH1, 0x5361, "rValue", system_sensors[1].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH1, 0x5362, "wGain",  &system_sensors[1].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH1, 0x5363, "wOffset",&system_sensors[1].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

// Channel 2 (VH)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH2, system_sensors[2].name, &meas_cb_2);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH2, 0x5371, "rValue", system_sensors[2].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH2, 0x5372, "wGain",  &system_sensors[2].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH2, 0x5373, "wOffset",&system_sensors[2].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

// Channel 3 (I1)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH3, system_sensors[3].name, &meas_cb_3);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH3, 0x5381, "rValue", system_sensors[3].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH3, 0x5382, "wGain",  &system_sensors[3].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH3, 0x5383, "wOffset",&system_sensors[3].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

// Channel 4 (I2)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH4, system_sensors[4].name, &meas_cb_4);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH4, 0x5391, "rValue", system_sensors[4].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH4, 0x5392, "wGain",  &system_sensors[4].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH4, 0x5393, "wOffset",&system_sensors[4].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

// Channel 5 (IH)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH5, system_sensors[5].name, &meas_cb_5);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH5, 0x53A1, "rValue", system_sensors[5].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH5, 0x53A2, "wGain",  &system_sensors[5].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH5, 0x53A3, "wOffset",&system_sensors[5].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

#ifdef CONFIG_SHIELD_OWNVERTER
// Channel 6 (V3)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH6, system_sensors[6].name, &meas_cb_6);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH6, 0x53B1, "rValue", system_sensors[6].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH6, 0x53B2, "wGain",  &system_sensors[6].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH6, 0x53B3, "wOffset",&system_sensors[6].offset, 4, THINGSET_ANY_RW, SUBSET_SER);

// Channel 7 (I3)
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CH7, system_sensors[7].name, &meas_cb_7);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH7, 0x53C1, "rValue", system_sensors[7].address, 2, THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH7, 0x53C2, "wGain",  &system_sensors[7].gain, 4, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CH7, 0x53C3, "wOffset",&system_sensors[7].offset, 4, THINGSET_ANY_RW, SUBSET_SER);
#endif


    // Group for Meas ParamMap
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_PARAMMAP, "rParamMap", THINGSET_NO_CALLBACK);



THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_V1,
    "V1", &meas_v1, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_V2,
    "V2", &meas_v2, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_VH,
    "VH", &meas_vh, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_I1,
    "I1", &meas_i1, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_I2,
    "I2", &meas_i2, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_IH,
    "IH", &meas_ih, THINGSET_ANY_R, SUBSET_SER);
#ifdef CONFIG_SHIELD_OWNVERTER
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_V3,
    "V3", &meas_v3, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_UINT8(ID_MEAS_PARAMMAP, ID_MEAS_PARAM_I3,
    "I3", &meas_i3, THINGSET_ANY_R, SUBSET_SER);
#endif




#endif /* DATA_OBJECTS_H */
