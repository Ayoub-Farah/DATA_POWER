####################################################################################
# TWR : this file contains variables that are modifiable by front/back and used    #
# by front/back. It permits the exchange of informations between front and back    #
# Author : TWR                                                                     #
####################################################################################

import serial.tools.list_ports

###################################################################
#                       GetCOMObjectPORT                          #
# Desc : this function is intended to handle the issue of serial  #
# port randomly changing                                          #
# Input : COMObjectDesc (str) : the COM object description we     #
# want to get the COM Port                                        #
# Output : the COM Port of the input                              #
###################################################################
def GetCOMObjectPORT(COMObjectDesc):
    # Get a list of all available serial ports
    available_ports = serial.tools.list_ports.comports()

    # Loop through all available ports and look for the one that matches the input COM port name
    for port in available_ports:
        descWithPort = port.description
        descWithoutPort = descWithPort.split('(COM')[0].strip()
        if descWithoutPort == COMObjectDesc:
            # Return the matching port name
            return port.device
    
    # If no match was found, return None
    return None

###################################################################
#                   Test progression bar                          #
###################################################################
TestCoefficientsProgress = 0
TestEfficiencyProgress = 0
TestAccuracyProgress = 0
TestHistogramProgress = 0

###################################################################
#                      Bench state machine                        #
###################################################################
#State of test bench
TestBenchStatus = 0

#State description
TestBenchStatusCode_0 = "Status: waiting for a test to be selected"
TestBenchStatusCode_1 = "Status: Coefficients test running"
TestBenchStatusCode_2 = "Status: Coefficients test done, waiting for instructions"
TestBenchStatusCode_3 = "Status: Accuracy test running"
TestBenchStatusCode_4 = "Status: Accuracy test done, waiting for instructions"
TestBenchStatusCode_5 = "Status: Efficiency (no temp meas) test running"
TestBenchStatusCode_6 = "Status: Efficiency (no temp meas) test done, waiting for instructions"
TestBenchStatusCode_7 = "Status: Histogram test running"
TestBenchStatusCode_8 = "Status: Histogram test done, waiting for instructions"
TestBenchStatusCode_9 = "Status: Efficiency (temp meas) test running"
TestBenchStatusCode_10 = "Status: Efficiency (temp meas) test done, waiting for instructions"

###################################################################
#                 Serial communication items                      #
###################################################################
#Active load serial comm parameters
ITE132CommPortDescWithoutPort = "Prolific USB-to-Serial Comm Port"
ActiveLoadCommPorttmp = GetCOMObjectPORT(ITE132CommPortDescWithoutPort)
ActiveLoadCommPort=ActiveLoadCommPorttmp  #COM28
ActiveLoadbaudRate = 9600
ActiveLoadbytesize = 8 #not used ?
ActiveLoadparity = "N"
ActiveLoadstopbits = 2 #not used ?
ActiveLoadsTimeout = 2
ActiveLoadAddress  = 0 #Address of active load (default is 0)

#Power supply serial comm parameters
PSUCommPortDescWithoutPort = "PSI 9000 DT Series"
PSUCommPorttmp = "COM5" #GetCOMObjectPORT(PSUCommPortDescWithoutPort)
PSUCommPort=PSUCommPorttmp

#STlink serial comm parameters
STLINKCommPortDescWithoutPort = "STMicroelectronics STLink Virtual COM Port"
STLINKCommPort = "COM18" #GetCOMObjectPORT(STLINKCommPortDescWithoutPort)
STLINKbaudRate = 115200
STLINKbytesize = 8
STLINKparity ="N"
STLINKstopbits = 1
STLINKTimeout_s = 2

#Twist board controlling the cooldown fan serial comm parameters
CooldownFanCommPortDescWithoutPort = "STMicroelectronics STLink Virtual COM Port"
CooldownFanCommPort = "COM16" #GetCOMObjectPORT(STLINKCommPortDescWithoutPort)
CooldownFanbaudRate = 115200
CooldownFanbytesize = 8
CooldownFanparity ="N"
CooldownFanstopbits = 1
CooldownFanTimeout_s = 2

#K2000 temperature measurement
K2000CommPortDescWithoutPort = "USB-SERIAL CH340"
TempMeasCommPorttmp = GetCOMObjectPORT(K2000CommPortDescWithoutPort)
#TempMeasCommPort=TempMeasCommPorttmp
TempMeasCommPort="COM11"
TempMeasbaudRate = 9600
TempMeasbytesize = 8 #Not used ?
TempMeasparity = "N"
TempMeasstopbits = 1 #Not used ?
TempMeasTimeout = 2
TempMeasAddress  = 0 #Address of temperature measurment (default is 0), #Not used ?

DynamicCOMPORT = "COM15"
DynamicNVSSAVE = 0

###################################################################
#                 GPIB Instruments addresses                      #
###################################################################
IHighGPIBAddress = 'GPIB0::1::INSTR'
VHighGPIBAddress = 'GPIB0::2::INSTR'
VLowGPIBAddress = 'GPIB0::3::INSTR'
ILowGPIBAddress = 'GPIB0::4::INSTR'
