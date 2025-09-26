####################################################################################
# TWR : File containing functions to measure temperature with K2000 multimeter     #
# declared as a serial object.                                                     #
# Author : LFLAVADO                                                                #
####################################################################################

#Python modules import
import time

###################################################################
#                               Init                              #
###################################################################
def Init(TempObj, baudrate, parity, timeout):
    
    TempObj.baudrate = baudrate
    TempObj.parity = parity
    TempObj.timeout = timeout
    TempObj.open    
    TempObj.write(b"*rst; *cls\r")
    TempObj.write(b"sense:temp:tc:type k\r")
    TempObj.write(b"sense:temp:tc:rjun:rsel SIM\r")
    TempObj.write(b"sense:temp:tc:rjun:sim 26\r")

###################################################################
#                         GetTemp_C                               #
###################################################################
def GetTemp_C(TempObj):    

    TempObj.write(b"meas:temp?\r")
    temp_meas = float(TempObj.readline())

    return temp_meas