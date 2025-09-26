####################################################################################
# TWR : EAPSI9750 power supply class                                               #
# Author : TWR                                                                     #
####################################################################################

#Python modules import
import time

class EAPSI9750:
    def __init__(self, ress):
        self.ress = ress
        ress.write("*CLS")

    def exit(self):
        self.ress.write('SOURce:VOLTage 20')
        time.sleep(0.5)
        self.ress.write('OUTput OFF')
        self.ress.write('SYST:LOCK OFF')

    def powerON(self):
        self.ress.write('OUTput ON')
        time.sleep(0.5)

    def powerOFF(self):
        self.ress.write('OUTput OFF')
        time.sleep(0.5)


    def init(self):
        self.ress.write('SYST:LOCK ON')
        time.sleep(0.5)

    def setVoltage_mV(self, Voltage_mV):
        print("Setting source voltage")
        Voltage_V = round(Voltage_mV/1000, 1)
        Voltage_V = str(Voltage_V)
        Msg = 'SOURce:VOLTage ' + Voltage_V
        self.ress.write(Msg)

    def setVoltage_V(self, Voltage_V):
        Voltage_V = str(Voltage_V)
        Msg = 'SOURce:VOLTage ' + Voltage_V
        self.ress.write(Msg)

    def setCurrentLimit_A(self, Current_A):
        Current_A = str(Current_A)
        # Msg = 'SOURce:CURRent:PROTection ' + Current_A
        Msg = 'SOURce:CURRent ' + Current_A
        self.ress.write(Msg)


    def exit(self):
        self.ress.write('SOURce:VOLTage 20')
        time.sleep(0.5)
        self.ress.write('OUTput OFF')
        self.ress.write('SYST:LOCK OFF')