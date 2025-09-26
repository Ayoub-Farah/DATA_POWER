####################################################################################
# TWR : K2000 multimeter class                                                     #
# Author : TWR                                                                     #
####################################################################################

#Python modules import
import ssl
import time
import pyvisa

class KEITHLEY2000:
    def __init__(self, ress):
        self.ress = ress
        ress.write("*rst; status:preset; *cls")

    ###################################################################
    #                   getAveragedVoltage_mV                         #
    ###################################################################
    def getAveragedVoltage_mV(self, number_of_readings, interval_in_ms):

        Value=0
        Avg_Value=0

        # self.ress.write("*rst; status:preset; *cls")
        self.ress.write("*rst")
        self.ress.write("status:measurement:enable 512; *sre 1")
        self.ress.write("sample:count %d" % number_of_readings)
        self.ress.write("trigger:source bus")
        self.ress.write("trigger:delay %f" % (interval_in_ms / 1000.0))
        self.ress.write("trace:points %d" % number_of_readings)
        self.ress.write("trace:feed sense1; feed:control next")

        self.ress.write("initiate")
        self.ress.assert_trigger()
        self.ress.wait_for_srq()

        Value = self.ress.query_ascii_values("trace:data?")
        Avg_Value = (sum(Value) / len(Value))
        Avg_Value_mV = Avg_Value*1000
        Avg_Value_mV = Avg_Value_mV

        self.ress.query("status:measurement?")
        self.ress.write("trace:clear; feed:control next")

        return Avg_Value_mV

    ###################################################################
    #                   getAveragedVoltage_mV                         #
    ###################################################################
    def getAveragedVoltage_V(self, number_of_readings, interval_in_ms):

        Value=0
        Avg_Value_V=0

        # self.ress.write("*rst; status:preset; *cls")
        self.ress.write("*rst")
        self.ress.write("status:measurement:enable 512; *sre 1")
        self.ress.write("sample:count %d" % number_of_readings)
        self.ress.write("trigger:source bus")
        self.ress.write("trigger:delay %f" % (interval_in_ms / 1000.0))
        self.ress.write("trace:points %d" % number_of_readings)
        self.ress.write("trace:feed sense1; feed:control next")

        self.ress.write("initiate")
        self.ress.assert_trigger()
        self.ress.wait_for_srq()

        Value = self.ress.query_ascii_values("trace:data?")
        Avg_Value_V = (sum(Value) / len(Value))

        self.ress.query("status:measurement?")
        self.ress.write("trace:clear; feed:control next")

        return Avg_Value_V

    ###################################################################
    #                          GetTemp_C                              #
    ###################################################################
    def GetTemp_C(self):

        # self.ress.write("*rst; status:preset; *cls")
        self.ress.write("*rst")
        self.ress.write("sense:temp:tc:type k")
        self.ress.write("sense:temp:tc:rjun:rsel SIM")
        self.ress.write("sense:temp:tc:rjun:sim 26")

        temp_meas = self.ress.write("meas:temp?")

        return temp_meas

    ###################################################################
    #                      GetRes2Wire_Ohm                            #
    ###################################################################
    def GetRes2Wire_Ohm(self):

        self.ress.write("*rst; status:preset; *cls")
        # self.ress.write("*rst")
        # self.ress.write("CONF:RES")
        # self.ress.write("RES:RANG 100")
        self.ress.write("SAMP:COUN 20")

        temp_meas = self.ress.write("read?")

        return temp_meas
