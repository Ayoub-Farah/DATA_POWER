####################################################################################
# TWR : File containing functions related to power measures/conversion             #
# Author : TWR                                                                     #
####################################################################################

###################################################################
#                    ShuntVoltage_mVToCurr_mA                     #
###################################################################
def ShuntVoltage_mVToCurr_mA(Voltage_mV, shunt_in_milliohm, CorrectionRatio):
    """
    Converts shunt voltage in millivolts to current in milliamps.

    :param Voltage_mV: The shunt voltage in millivolts.
    :type Voltage_mV: float
    :param shunt_in_milliohm: The shunt resistance in milliohms.
    :type shunt_in_milliohm: float
    :param CorrectionRatio: The correction ratio.
    :type CorrectionRatio: float
    :return: The current in milliamps.
    :rtype: float
    """

    current_mA = (abs(Voltage_mV/(shunt_in_milliohm/1000))*CorrectionRatio)
    current_mA = round(current_mA, 3)

    return current_mA

###################################################################
#                    ShuntVoltage_mVToCurr_A                     #
###################################################################
def ShuntVoltage_mVToCurr_A(Voltage_mV, shunt_in_milliohm, CorrectionRatio):
    """
    Converts shunt voltage in millivolts to current in amps.

    :param Voltage_mV: The shunt voltage in millivolts.
    :type Voltage_mV: float
    :param shunt_in_milliohm: The shunt resistance in milliohms.
    :type shunt_in_milliohm: float
    :param CorrectionRatio: The correction ratio.
    :type CorrectionRatio: float
    :return: The current in amps.
    :rtype: float
    """
    current_mA = (abs(Voltage_mV/(shunt_in_milliohm/1000))*CorrectionRatio)
    current_A = round(current_mA/1000, 6)

    return current_A


###################################################################
#                     ComputePowerInWatt                          #
###################################################################
def ComputePowerInWatt(Voltage_in_Volt, Current_in_AMP):
    """
    Computes power in watts given voltage in volts and current in amperes.

    :param Voltage_in_Volt: The voltage in volts.
    :type Voltage_in_Volt: float
    :param Current_in_AMP: The current in amperes.
    :type Current_in_AMP: float
    :return: The power in watts.
    :rtype: float
    """
    Power_W = Voltage_in_Volt*Current_in_AMP
    Power_W = round(Power_W,3)

    return(Power_W)

###################################################################
#                       ComputeEff_Per                            #
###################################################################
def ComputeEff_Per(PLow_W, PHigh_W):
    """
    Computes efficiency percentage given low and high power values.

    :param PLow_W: The low power value in watts.
    :type PLow_W: float
    :param PHigh_W: The high power value in watts.
    :type PHigh_W: float
    :return: The efficiency percentage.
    :rtype: float
    """
    if (PHigh_W==0):
        PHigh_W=9999#To avoid null division error

    Eff = PLow_W/PHigh_W
    Eff_Per = round((Eff*100), 3)

    return Eff_Per

###################################################################
#                     ComputePowerResults                         #
###################################################################
def ComputePowerResults(DataDMM):
    """
    Computes power results including high power, low power, and efficiency.

    :param DataDMM: List containing voltage and current measurements.
    :type DataDMM: list[float]
    :return: List containing high power, low power, and efficiency.
    :rtype: list[float]
    """
    #initialize power array
    PowerList_W = [0]*3

    VHigh_V = DataDMM[0]/1000
    IHigh_A = DataDMM[1]/1000
    VLow_V = DataDMM[2]/1000
    ILow_A = DataDMM[3]/1000

    #Compute power from voltage and current
    PowerHigh_W=ComputePowerInWatt(VHigh_V, IHigh_A)
    PowerLow_W=ComputePowerInWatt(VLow_V, ILow_A)

    #Compute efficiency
    Efficiency_Per = ComputeEff_Per(PowerLow_W, PowerHigh_W)

    PowerList_W[0] = PowerHigh_W
    PowerList_W[1] = PowerLow_W
    PowerList_W[2] = Efficiency_Per

    return PowerList_W