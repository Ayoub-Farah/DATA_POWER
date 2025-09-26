####################################################################################
#   This file contains functions to drive the IT8512B ITECH DC ELECTRONIC LOAD     #
# Author : TWR                                                                     #
####################################################################################

#Python modules import
import bk8500, time

###################################################################
#                               Init                              #
###################################################################
def Init(SerialObj, baudrate, parity, timeout):
    
    SerialObj.baudrate = baudrate
    SerialObj.parity = parity
    SerialObj.timeout = timeout
    SerialObj.open

###################################################################
#                          SetCurrent_A                           #
###################################################################
def SetCurrent_A(SerialObj, SerialAdd, Current_A):

    # bk8500.set_remote(SerialObj, SerialAdd, True)
    # bk8500.enable_input(SerialObj, SerialAdd, True)
    bk8500.set_current(SerialObj, SerialAdd, Current_A)
    time.sleep(0.2)

###################################################################
#                          SetVoltage_V                           #
###################################################################
def SetRemoteMode(SerialObj, SerialAdd, RemoteMode):

    bk8500.set_remote(SerialObj, SerialAdd, RemoteMode)

###################################################################
#                          SetVoltage_A                           #
###################################################################
def SetEnableInput(SerialObj, SerialAdd, EnableInput):

    bk8500.enable_input(SerialObj, SerialAdd, EnableInput)


###################################################################
#                          SetVoltage_A                           #
###################################################################
def SetVoltage_V(SerialObj, SerialAdd, Voltage_V):

    # bk8500.set_remote(SerialObj, SerialAdd, True)
    # bk8500.enable_input(SerialObj, SerialAdd, True)
    bk8500.set_voltage(SerialObj, SerialAdd, Voltage_V)
    time.sleep(0.2)

###################################################################
#                          SetResistance_A                           #
###################################################################
def SetResistance_Ohm(SerialObj, SerialAdd, Resistance_Ohm):

    # bk8500.set_remote(SerialObj, SerialAdd, True)
    # bk8500.enable_input(SerialObj, SerialAdd, True)
    bk8500.set_resistance(SerialObj, SerialAdd, Resistance_Ohm)
    time.sleep(0.2)


def SetMode(Serial, Add, Mode):
    '''
    SetSafetySettings Function:
    
    This function sets the mode of the device.
    
    Modes:
        - "CC": Constant Current mode
        - "CV": Constant Voltage mode
        - "CW": Constant Power mode
        - "CR": Constant Resistance mode
    
    Parameters:
        - Serial: Serial connection to the device
        - Add: Address of the device
        - Mode: Desired mode ("CC", "CV", "CW", or "CR")
    '''
    bk8500.set_mode(Serial, Add, Mode)

###################################################################
#                       SetSafetySettings                         #
###################################################################
def SetSafetySettings(Serial, Add, MaxVolt_V, MaxCurr_A, MaxPower_W):

    # #set remote
    # bk8500.set_remote(Serial, Add, True)

    #Set absolute safety values
    bk8500.set_max_voltage(Serial, Add, MaxVolt_V)
    bk8500.set_max_current(Serial, Add, MaxCurr_A)
    bk8500.set_max_power(Serial, Add, MaxPower_W)
    # bk8500.set_mode(Serial, Add, "CC")