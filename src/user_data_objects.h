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


extern uint8_t mode;

/* Temporary storage fore measured value (ctrl task) */
static float32_t meas_data;


typedef struct {
    const char *name;
    const float32_t *address;      /* pointer to the live signal */
    sensor_t    channel_reference;
} TrackingVariables;

static const TrackingVariables tracking_vars[] = {
    { "V1", &V1_low_value, V1_LOW },
    { "V2", &V2_low_value, V2_LOW },
    { "VH", &V_high_value, V_HIGH },
    { "I1", &I1_low_value, I1_LOW },
    { "I2", &I2_low_value, I2_LOW },
    { "IH", &I_high_value, I_HIGH },
#ifdef CONFIG_SHIELD_OWNVERTER
    { "V3", &V3_low_value, V3_LOW },
    { "I3", &I3_low_value, I3_LOW },
#endif
};


/* =========================================================================
 * Backing storage
 * ========================================================================= */

/* ---- Driver (global) ---- */
static bool driver_wEnable = false;     /* global driver enable (legacy POWER_ON/OFF) */
static bool driver_rStatus = false;     /* global driver status (readback) */

/* ---- Power / Legs ----
 * These fields cover the legacy downlink actions:
 *  LEG/CAPA/DRIVER/BUCK/BOOST/REFERENCE/DUTY/PHASE_SHIFT/DEAD_TIME_RISING/DEAD_TIME_FALLING/FREQUENCY
 */
typedef struct {
    /* Selection + reference */
    int8_t  wVar;            /* index into tracking_vars[] (0..TRACKING_VARS_COUNT-1) */
    float32_t   wRef;            /* setpoint for selected variable (REFERENCE) */
    float32_t       rVarValue;   /* live value of selected tracking variable */
    const char *rVarName;    /* name of selected variable */

    /* State toggles */
    bool    wLeg;            /* LEG on/off */
    bool    wCapa;           /* CAPA on/off */
    bool    wDriver;         /* DRIVER on/off (per-leg) */
    bool    wBuck;           /* BUCK mode enable */
    bool    wBoost;          /* BOOST mode enable */

    /* PWM-related knobs (kept in leg group for compatibility) */
    float32_t   wDuty;           /* DUTY [0..1] */
    float32_t   wPhase_deg;      /* PHASE_SHIFT [deg] */
    uint16_t wDead_rise_ns;  /* DEAD_TIME_RISING [ns] */
    uint16_t wDead_fall_ns;  /* DEAD_TIME_FALLING [ns] */
    float32_t   wFreq_Hz;        /* FREQUENCY [Hz] (may be routed to shared timer) */

} power_leg_t;

static power_leg_t power_legs[POWER_NUM_LEGS] = {
    { .wVar = 0, .wRef = 0.0f, .rVarValue=0.0f, .rVarName=NULL, 
      .wLeg=false, .wCapa=false, .wDriver=false, .wBuck=false, .wBoost=false,
      .wDuty=0.0f, .wPhase_deg=0.0f, .wDead_rise_ns=0, .wDead_fall_ns=0, 
      .wFreq_Hz=0.0f },
    { .wVar = 1, .wRef = 0.0f, .rVarValue=0.0f, .rVarName=NULL, 
      .wLeg=false, .wCapa=false, .wDriver=false, .wBuck=false, .wBoost=false,
      .wDuty=0.0f, .wPhase_deg=0.0f, .wDead_rise_ns=0, .wDead_fall_ns=0, 
      .wFreq_Hz=0.0f },
#ifdef CONFIG_SHIELD_OWNVERTER
    { .wVar = 6, .wRef = 0.0f, .wLeg=false, .wCapa=false, .wDriver=false, .wBuck=false, .wBoost=false,
        .wDuty=0.0f, .wPhase_deg=0.0f, .wDead_rise_ns=0, .wDead_fall_ns=0, .wFreq_Hz=0.0f,
        .rVarValue=0.0f, .rVarName=NULL },
#endif

};

/* ---- Measurements (single analog channel + calibration) ---- */
static float32_t meas_rANALOG = 0.0f;   /* calibrated analog value (read-only) */
static float32_t meas_sOffset = 0.0f;   /* persistent calibration offset */
static float32_t meas_sGain   = 1.0f;   /* persistent calibration gain */

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
#define ID_CONF             0x010
#define ID_POWER            0x100
#define ID_MEAS             0x300
#define ID_MEAS_CAL         0x510
#define ID_COMM             0x700
#define ID_COMM_SYNC        0x710
#define ID_COMM_ANALOG      0x7220

/* Power/Legs sub-groups & items (LEG1) */
#define ID_PWR_LEG1         0x110
#define ID_PWR_LEG1_WVAR    0x111
#define ID_PWR_LEG1_WREF    0x112
#define ID_PWR_LEG1_WLEG    0x113
#define ID_PWR_LEG1_WCAPA   0x114
#define ID_PWR_LEG1_WDRV    0x115
#define ID_PWR_LEG1_WBUCK   0x116
#define ID_PWR_LEG1_WBOOST  0x117
#define ID_PWR_LEG1_WDUTY   0x118
#define ID_PWR_LEG1_WPHASE  0x119
#define ID_PWR_LEG1_WDRISE  0x11A
#define ID_PWR_LEG1_WDFALL  0x11B
#define ID_PWR_LEG1_WFREQ   0x11C
#define ID_PWR_LEG1_RVAL    0x11D
#define ID_PWR_LEG1_RNAME   0x11E

/* Power/Legs sub-groups & items (LEG2) */
#define ID_PWR_LEG2         0x120
#define ID_PWR_LEG2_WVAR    0x121
#define ID_PWR_LEG2_WREF    0x122
#define ID_PWR_LEG2_WLEG    0x123
#define ID_PWR_LEG2_WCAPA   0x124
#define ID_PWR_LEG2_WDRV    0x125
#define ID_PWR_LEG2_WBUCK   0x126
#define ID_PWR_LEG2_WBOOST  0x127
#define ID_PWR_LEG2_WDUTY   0x128
#define ID_PWR_LEG2_WPHASE  0x129
#define ID_PWR_LEG2_WDRISE  0x12A
#define ID_PWR_LEG2_WDFALL  0x12B
#define ID_PWR_LEG2_WFREQ   0x12C
#define ID_PWR_LEG2_RVAL    0x12D
#define ID_PWR_LEG2_RNAME   0x12E

#ifdef CONFIG_SHIELD_OWNVERTER
    /* Power/Legs sub-groups & items (LEG3) */
    #define ID_PWR_LEG3         0x130
    #define ID_PWR_LEG3_WVAR    0x131
    #define ID_PWR_LEG3_WREF    0x132
    #define ID_PWR_LEG3_WLEG    0x133
    #define ID_PWR_LEG3_WCAPA   0x134
    #define ID_PWR_LEG3_WDRV    0x135
    #define ID_PWR_LEG3_WBUCK   0x136
    #define ID_PWR_LEG3_WBOOST  0x137
    #define ID_PWR_LEG3_WDUTY   0x138
    #define ID_PWR_LEG3_WPHASE  0x139
    #define ID_PWR_LEG3_WDRISE  0x13A
    #define ID_PWR_LEG3_WDFALL  0x13B
    #define ID_PWR_LEG3_WFREQ   0x13C
    #define ID_PWR_LEG3_RVAL    0x13D
    #define ID_PWR_LEG3_RNAME   0x13E
#endif

/* Measurement items */
#define ID_MEAS_RANALOG     0x501
#define ID_MEAS_SOFFSET     0x511
#define ID_MEAS_SGAIN       0x512
#define ID_MEAS_XCAL        0x513

/* Measurements → individual items */
#define ID_MEAS_V1_LOW      0x502
#define ID_MEAS_V2_LOW      0x503
#define ID_MEAS_V_HIGH      0x504
#define ID_MEAS_I1_LOW      0x505
#define ID_MEAS_I2_LOW      0x506
#define ID_MEAS_I_HIGH      0x507
#define ID_MEAS_TEMP1       0x508
#define ID_MEAS_TEMP2       0x509
#ifdef CONFIG_SHIELD_OWNVERTER
    #define ID_MEAS_V3_LOW      0x50A
    #define ID_MEAS_I3_LOW      0x50B
#endif

/* Communication items */
#define ID_COMM_SYNC_RROLE  0x711
#define ID_COMM_ANALOG_WEN  0x721
#define ID_COMM_ANALOG_RVAL 0x722

/* =========================================================================
 * Groups
 * ========================================================================= */

THINGSET_ADD_GROUP(TS_ID_ROOT, ID_CONF,"Config", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(TS_ID_ROOT, ID_POWER,"Power",        THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(TS_ID_ROOT, ID_MEAS,"Measurements", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_MEAS, ID_MEAS_CAL,"Cal",          THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(TS_ID_ROOT, ID_COMM,"Communication", THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_COMM, ID_COMM_SYNC,"Sync",         THINGSET_NO_CALLBACK);
THINGSET_ADD_GROUP(ID_COMM, ID_COMM_ANALOG,"Analog",       THINGSET_NO_CALLBACK);

/* =========================================================================
 * Power → Leg 1
 * ========================================================================= */
THINGSET_ADD_GROUP( ID_POWER, ID_PWR_LEG1, "LEG1", THINGSET_NO_CALLBACK );
THINGSET_ADD_ITEM_INT8 (ID_PWR_LEG1, ID_PWR_LEG1_WVAR,   "wVar", &power_legs[0].wVar,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG1, ID_PWR_LEG1_WREF,   "wRef", &power_legs[0].wRef, 3,     THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG1, ID_PWR_LEG1_WLEG,   "wLeg", &power_legs[0].wLeg,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG1, ID_PWR_LEG1_WCAPA,  "wCapa",&power_legs[0].wCapa,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG1, ID_PWR_LEG1_WDRV,   "wDriver",&power_legs[0].wDriver,   THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG1, ID_PWR_LEG1_WBUCK,  "wBuck",&power_legs[0].wBuck,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG1, ID_PWR_LEG1_WBOOST, "wBoost",&power_legs[0].wBoost,     THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG1, ID_PWR_LEG1_WDUTY,  "wDuty",&power_legs[0].wDuty, 5,    THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG1, ID_PWR_LEG1_WPHASE, "wPhase_deg",&power_legs[0].wPhase_deg, 1, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_PWR_LEG1, ID_PWR_LEG1_WDRISE,"wDead_rise_ns",&power_legs[0].wDead_rise_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_PWR_LEG1, ID_PWR_LEG1_WDFALL,"wDead_fall_ns",&power_legs[0].wDead_fall_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG1, ID_PWR_LEG1_WFREQ,  "wFreq_Hz",&power_legs[0].wFreq_Hz, 0, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG1, ID_PWR_LEG1_RVAL,   "rVarValue",&power_legs[0].rVarValue, 4, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_STRING(ID_PWR_LEG1, ID_PWR_LEG1_RNAME, "rVarName", (char*)&power_legs[0].rVarName, 4, THINGSET_ANY_R, SUBSET_SER);

/* =========================================================================
 * Power → Leg 2
 * ========================================================================= */
THINGSET_ADD_GROUP( ID_POWER, ID_PWR_LEG2, "LEG2", THINGSET_NO_CALLBACK );
THINGSET_ADD_ITEM_INT8 (ID_PWR_LEG2, ID_PWR_LEG2_WVAR,   "wVar", &power_legs[1].wVar,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG2, ID_PWR_LEG2_WREF,   "wRef", &power_legs[1].wRef, 3,     THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG2, ID_PWR_LEG2_WLEG,   "wLeg", &power_legs[1].wLeg,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG2, ID_PWR_LEG2_WCAPA,  "wCapa",&power_legs[1].wCapa,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG2, ID_PWR_LEG2_WDRV,   "wDriver",&power_legs[1].wDriver,   THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG2, ID_PWR_LEG2_WBUCK,  "wBuck",&power_legs[1].wBuck,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG2, ID_PWR_LEG2_WBOOST, "wBoost",&power_legs[1].wBoost,     THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG2, ID_PWR_LEG2_WDUTY,  "wDuty",&power_legs[1].wDuty, 5,    THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG2, ID_PWR_LEG2_WPHASE, "wPhase_deg",&power_legs[1].wPhase_deg, 1, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_PWR_LEG2, ID_PWR_LEG2_WDRISE,"wDead_rise_ns",&power_legs[1].wDead_rise_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_PWR_LEG2, ID_PWR_LEG2_WDFALL,"wDead_fall_ns",&power_legs[1].wDead_fall_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG2, ID_PWR_LEG2_WFREQ,  "wFreq_Hz",&power_legs[1].wFreq_Hz, 0, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG2, ID_PWR_LEG2_RVAL,   "rVarValue",&power_legs[1].rVarValue, 4, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_STRING(ID_PWR_LEG2, ID_PWR_LEG2_RNAME, "rVarName", (char*)&power_legs[1].rVarName, 4, THINGSET_ANY_R, SUBSET_SER);


/* =========================================================================
 * Power → Leg 3 - In the case of the ownverter
 * ========================================================================= */
#ifdef CONFIG_SHIELD_OWNVERTER

 THINGSET_ADD_GROUP( ID_POWER, ID_PWR_LEG3, "LEG3", THINGSET_NO_CALLBACK );
THINGSET_ADD_ITEM_INT8 (ID_PWR_LEG3, ID_PWR_LEG3_WVAR,   "wVar", &power_legs[1].wVar,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG3, ID_PWR_LEG3_WREF,   "wRef", &power_legs[1].wRef, 3,     THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG3, ID_PWR_LEG3_WLEG,   "wLeg", &power_legs[1].wLeg,        THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG3, ID_PWR_LEG3_WCAPA,  "wCapa",&power_legs[1].wCapa,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG3, ID_PWR_LEG3_WDRV,   "wDriver",&power_legs[1].wDriver,   THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG3, ID_PWR_LEG3_WBUCK,  "wBuck",&power_legs[1].wBuck,       THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_BOOL (ID_PWR_LEG3, ID_PWR_LEG3_WBOOST, "wBoost",&power_legs[1].wBoost,     THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG3, ID_PWR_LEG3_WDUTY,  "wDuty",&power_legs[1].wDuty, 5,    THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG3, ID_PWR_LEG3_WPHASE, "wPhase_deg",&power_legs[1].wPhase_deg, 1, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_PWR_LEG3, ID_PWR_LEG3_WDRISE,"wDead_rise_ns",&power_legs[1].wDead_rise_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_UINT16(ID_PWR_LEG3, ID_PWR_LEG3_WDFALL,"wDead_fall_ns",&power_legs[1].wDead_fall_ns, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG3, ID_PWR_LEG3_WFREQ,  "wFreq_Hz",&power_legs[1].wFreq_Hz, 0, THINGSET_ANY_RW, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_PWR_LEG3, ID_PWR_LEG3_RVAL,   "rVarValue",&power_legs[1].rVarValue, 4, THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_STRING(ID_PWR_LEG3, ID_PWR_LEG3_RNAME, "rVarName", (char*)&power_legs[1].rVarName, 4, THINGSET_ANY_R, SUBSET_SER);
#endif


/* =========================================================================
 * Measurements
 * ========================================================================= */
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_RANALOG, 
                        "rANALOG", &meas_rANALOG, 4,
                        THINGSET_ANY_R,  SUBSET_SER );
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CAL, ID_MEAS_SOFFSET, 
                        "sOffset", &meas_sOffset, 6,
                        THINGSET_ANY_RW, SUBSET_SER );
THINGSET_ADD_ITEM_FLOAT(ID_MEAS_CAL, ID_MEAS_SGAIN,   
                        "sGain",   &meas_sGain,   6,
                        THINGSET_ANY_RW, SUBSET_SER );

THINGSET_ADD_FN_VOID( ID_MEAS, ID_MEAS_XCAL, "xCalibrate", &Measurements_xCalibrate,
                      THINGSET_ANY_RW );

/* ---- Individual measurements (low-voltage rails, high-voltage, currents, temps) ---- */
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_V1_LOW, 
                        "rV1Low_V",  &V1_low_value, 2,
                        THINGSET_ANY_R, TS_SUBSET_LIVE);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_V2_LOW, 
                        "rV2Low_V",  &V2_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_V_HIGH,"rVHigh_V",   &V_high_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);

THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_I1_LOW, "rI1Low_A",  &I1_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_I2_LOW, "rI2Low_A",  &I2_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_I_HIGH,"rIHigh_A",   &I_high_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);

THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_TEMP1,  "rTemp_degC",   &temp_1_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_TEMP2,  "rTemp2_degC",  &temp_2_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);

/* ---- Extra channels present on OWNVERTER builds ---- */
#ifdef CONFIG_SHIELD_OWNVERTER
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_V3_LOW, "rV3Low_V",  &V3_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
THINGSET_ADD_ITEM_FLOAT(ID_MEAS, ID_MEAS_I3_LOW, "rI3Low_A",  &I3_low_value, 2,
                        THINGSET_ANY_R, SUBSET_SER);
#endif


/* =========================================================================
 * Communication
 * ========================================================================= */
THINGSET_ADD_ITEM_INT8 (ID_COMM_SYNC,   ID_COMM_SYNC_RROLE, "rRole",  &comm_sync_rRole,
                        THINGSET_ANY_R, SUBSET_SER );
THINGSET_ADD_ITEM_BOOL (ID_COMM_ANALOG, ID_COMM_ANALOG_WEN, "wEnable",&comm_analog_wEnable,
                        THINGSET_ANY_RW, SUBSET_SER );
THINGSET_ADD_ITEM_FLOAT(ID_COMM_ANALOG, ID_COMM_ANALOG_RVAL,"rValue", &comm_analog_rValue, 4,
                        THINGSET_ANY_R,  SUBSET_SER );


#endif /* DATA_OBJECTS_H */
