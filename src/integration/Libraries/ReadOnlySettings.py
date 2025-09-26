####################################################################################
# TWR : # This file contains variable that are not modifiable neither by front     #
# nor end.                                                                         #
# They can be used by front, back, or both.                                        #
# Author : TWR                                                                     #
####################################################################################

#*****************************************************************
#*****************************************************************
#                      Generic parameters                        #
#*****************************************************************
#*****************************************************************

###################################################################
#                 Serial communication items                      #
###################################################################
#Active load serial comm parameters

#*****************************************************************
#*****************************************************************
#                  End of generic parameters                     #
#*****************************************************************
#*****************************************************************

#*****************************************************************
#*****************************************************************
#                    Coefficients related                        #
#*****************************************************************
#*****************************************************************

###################################################################
#                           Headers                               #
###################################################################

CoeffHeaderVHigh = ['DMM VHigh (mV)', 'TWIST Duty Cycle', 'TWIST VHigh (quantum)', 'TWIST VLow1 (quantum)',\
    'TWIST VLow2 (quantum)', 'TWIST IHigh (quantum)', 'TWIST ILow1 (quantum)', 'TWIST ILow2 (quantum)']

CoeffHeaderIHigh = ['DMM IHigh (mA)', 'TWIST Duty Cycle', 'TWIST VHigh (quantum)', 'TWIST VLow1 (quantum)',\
    'TWIST VLow2 (quantum)', 'TWIST IHigh (quantum)', 'TWIST ILow1 (quantum)', 'TWIST ILow2 (quantum)']

CoeffHeaderVLow = ['DMM VLow (mV)', 'TWIST Duty Cycle', 'TWIST VHigh (quantum)', 'TWIST VLow1 (quantum)',\
    'TWIST VLow2 (quantum)', 'TWIST IHigh (quantum)', 'TWIST ILow1 (quantum)', 'TWIST ILow2 (quantum)']

CoeffHeaderILow = ['DMM ILow (mA)', 'TWIST Duty Cycle', 'TWIST VHigh (quantum)', 'TWIST VLow1 (quantum)',\
    'TWIST VLow2 (quantum)', 'TWIST IHigh (quantum)', 'TWIST ILow1 (quantum)', 'TWIST ILow2 (quantum)']

# ###################################################################
# #                      Duty step in duty sweep                    #
# ###################################################################
CoeffNumberOfStepInDutySweep = 5 # The number of times we increase D for ONE Duty sweep. 
# # Example : if NumberOfUpSent = 10, the final duty cycle will be :
# # Final D =  starting_duty_cycle_Percent + NumberOfStepInDutySweep*duty_step_Percent
# # Final D = 5+5*10 = 50%
# # if NumberOfUpSent = 16, the final duty cycle will be 5*17 = 85%

###################################################################
#                    Calibration Coefficients                     #
###################################################################

CalibrationCoeffs = {
"gV1": "45.021",
"oV1": "-94364",
"r_sqV1": "0", 
"gV2": "45.021",
"oV2": "-94364",
"r_sqV2": "0", 
"gVH": "29.964",
"oVH": "-94364",
"r_sqVH": "0", 
"gI1": "5",
"oI1": "-10000",
"r_sqI1": "0",
"gI2": "5",
"oI2": "-10000",
"r_sqI2": "5", 
"gIH": "5",
"oIH": "-10000",
"r_sqIH": "0", 
}

#*****************************************************************
#*****************************************************************
#                End of Coefficients related                     #
#*****************************************************************
#*****************************************************************

#*****************************************************************
#*****************************************************************
#                     Efficiency related                         #
#*****************************************************************
#*****************************************************************

###################################################################
#                      Header of saved CSV                        #
###################################################################
SavedCSVHeader = ['Duty cycle (%)','DMM VHigh (mV)', 'DMM IHigh (mA)',\
'DMM VLow (mV)', 'DMM ILow (mA)', 'DMM POWER High (W)','DMM POWER Low (W)', 'DMM Efficiency (%)',\
'TWIST Duty Cycle (%)', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)', 'TWIST VLow2 (mV)', 'TWIST IHigh (mA)',\
'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

###################################################################
#                       Conversion ratio                          #
###################################################################
MilliToUnitConversionRatio = 1000

###################################################################
#                       Hardware config                           #
###################################################################
shunt_value_in_mOhm = 10 #The value of the shunt for high and low side current measure

IHighShuntCorrectionFactor = 1.033
ILowShuntCorrectionFactor = 1.039

###################################################################
#                         DMM settings                            #
###################################################################
DMM_READING_INTERVAL_ms = 1 #ms, the interval between each sampling point for the average of DMM measure
Number_of_DMM_READ_AVG = 2 #Number of samples to compute the average of DMM measure

###################################################################
#              Delays for steady state reach                      #
###################################################################
TimeWaitedForSteadyStateReach_Reset_ms = 2000

#*****************************************************************
#*****************************************************************
#                 End of Efficiency related                      #
#*****************************************************************
#*****************************************************************

#*****************************************************************
#*****************************************************************
#                       Accuracy related                         #
#*****************************************************************
#*****************************************************************

###################################################################
#                           Accuracy Header                      #
###################################################################
AccVHighHeader = ['DMM VHigh (mV)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

AccIHighHeader =  ['DMM IHigh (mA)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

AccVLowHeader = ['DMM VLow (mV)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

AccILowHeader = ['DMM ILow (mA)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

AccHeaderDict = {
    "VHigh": AccVHighHeader,
    "IHigh": AccIHighHeader,
    "VLow": AccVLowHeader,
    "ILow": AccILowHeader,
}

###################################################################
#                           Histogram Header                      #
###################################################################
HistVHighHeader = ['DMM VHigh (mV)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

HistIHighHeader =  ['DMM IHigh (mA)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

HistVLowHeader = ['DMM VLow (mV)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

HistILowHeader = ['DMM ILow (mA)', 'TWIST Duty Cycle', 'TWIST VHigh (mV)', 'TWIST VLow1 (mV)',\
'TWIST VLow2 (mV)', 'TWIST IHigh (mA)', 'TWIST ILow1 (mA)', 'TWIST ILow2 (mA)']

HistHeaderDict = {
    "VHigh": HistVHighHeader,
    "IHigh": HistIHighHeader,
    "VLow": HistVLowHeader,
    "ILow": HistILowHeader,
}

HistMeanStandardDeviationDict = {
    "VHigh (mV)" : {
        "Mean" : "",
        "StandartDeviation" : "",
    },
    "IHigh (mA)" : {
        "Mean" : "",
        "StandartDeviation" : "",
    },
    "VLow1 (mV)" : {
        "Mean" : "",
        "StandartDeviation" : "",
    },
    "VLow2 (mV)" : {
        "Mean" : "",
        "StandartDeviation" : "",
    },
    "ILow1 (mA)" : {
        "Mean" : "",
        "StandartDeviation" : "",
    },
    "ILow2 (mA)" : {
        "Mean" : "",
        "StandartDeviation" : "",
    },
}

#*****************************************************************
#*****************************************************************
#                 End of Histogram related                      #
#*****************************************************************
#*****************************************************************

#*****************************************************************
#*****************************************************************
#            Dynamic update coefficients related                 #
#*****************************************************************
#*****************************************************************

# ###################################################################
#            Dynamically update calibration coefficients tab        #
# ###################################################################

# Default calibration coefficients
DefaultGainV1 = 0.045
DefaultOffsetV1 = -94.364
DefaultGainV2 = 0.045
DefaultOffsetV2 = -94.364
DefaultGainVH = 0.029
DefaultOffsetVH = 0
DefaultGainI1 = 0.005
DefaultOffsetI1 = -10
DefaultGainI2 = 0.005
DefaultOffsetI2 = -10
DefaultGainIH = 0.005
DefaultOffsetIH = -10

# Theoretical calibration coefficients
TheoGainV1 = 0.045
TheoOffsetV1 = -94.364
TheoGainV2 = 0.045
TheoOffsetV2 = -94.364
TheoGainVH = 0.029
TheoOffsetVH = 0
TheoGainI1 = 0.005
TheoOffsetI1 = -10
TheoGainI2 = 0.005
TheoOffsetI2 = -10
TheoGainIH = 0.005
TheoOffsetIH = -10

#*****************************************************************
#*****************************************************************
#          End of Dynamic update coefficients related            #
#*****************************************************************
#*****************************************************************