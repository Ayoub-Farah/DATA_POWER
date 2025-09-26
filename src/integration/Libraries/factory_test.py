"""
Copyright (c) 2021-2024 LAAS-CNRS

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 2.1 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: LGLPV2.1
"""

"""
@brief  This is a class for the factory test of Twitst 1.4.1

@author Luiz Villa <luiz.villa@laas.fr>
@author Thomas Walter <thomas.walter@laas.fr>
"""

#Python modules import
import math as m
from scipy import signal
import argparse
import datetime
import struct
import os, csv, serial, pyvisa, time, sys, io
import numpy as np
from scipy.interpolate import interp1d
from scipy import stats
import cv2
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime 
# from mpl_toolkits.mplot3d import Axes3D

#Importing files of the project that are not in the same folder as this file
ScriptPath = os.path.dirname(os.path.abspath(__file__))
LibFolderName = "Libraries"
LibPath = os.path.join(ScriptPath, LibFolderName)
sys.path.insert(1, LibPath)



from Libraries import EAPSI9750, K2000, LoadControl, \
SPIN_Comm,TempMeas, Power, prog_utils, common, OwntechProdSdk, \
settings, progress_bar, ReadOnlySettings, FrontBackNoJsonSave, RecordedDatas
prod_sdk = OwntechProdSdk.OwntechProdSdk(settings.base_url)

IDLE = 0
RECORD = 1



# PI controller function
def PI_controller(err, integral, Kp, Ki, sampling_time):
    integral = integral + err * sampling_time
    duty_cycle = Kp * err + Ki * integral
    print('integral : ',integral, 'dc :', duty_cycle)
    return duty_cycle, integral

# Duty cycle saturation function
def saturate_duty_cycle(duty_cycle, saturation_limit_up, saturation_limit_down):
    if duty_cycle > saturation_limit_up:
        return saturation_limit_up
    elif duty_cycle < saturation_limit_down:
        return saturation_limit_down
    else:
        return duty_cycle


class FactoryTest:
    def __init__(self, TempMeas = True):
        

        self.ret_login = prod_sdk.login(settings.username, settings.password)

        self.boardSN = ""
        self.testNumber = 0

        self.test_flags = {"LEG1": {"capa": False, "driver": False, "nvs": False, "calib": False, "buck": False, "boost": False , "open_circuit": False, "efficiency": False},
                           "LEG2": {"capa": False, "driver": False, "nvs": False, "calib": False, "buck": False, "boost": False, "open_circuit": False, "efficiency": False},
                           "TWIST" : {"program": False, "feeder": False, "RS485": False, "Sync": False, "Analog": False, "CAN": False}}


        self.message_length = 16
        self.DUT_message_index = { "D1": {"index": 0},
                                "V1": {"index": 1},
                                "I1": {"index": 2},
                                "M1": {"index": 3},
                                "T1": {"index": 4},
                                "D2": {"index": 5},
                                "I2": {"index": 6},
                                "V2": {"index": 7},
                                "M2": {"index": 8},
                                "T2": {"index": 9},
                                "VH": {"index": 10},
                                "IH": {"index": 11},
                                "AN": {"index": 12},
                                "CE": {"index": 13},
                                "CR": {"index": 14},
                                "RS": {"index": 15}}

        # self.DUT_message_index = {"D1": {"index": 0},
        #                           "V1": {"index": 1},
        #                           "I1": {"index": 2},
        #                           "M1": {"index": 3},
        #                           "D2": {"index": 4},
        #                           "I2": {"index": 5},
        #                           "V2": {"index": 6},
        #                           "M2": {"index": 7},
        #                           "VH": {"index": 8},
        #                           "IH": {"index": 9},
        #                           "AN": {"index": 10},
        #                           "CE": {"index": 11},
        #                           "CR": {"index": 12},
        #                           "RS": {"index": 13}}

        self.program_test_result = False #TODO: check if Usefull ???
        self.feeder_test_result = False
        self.open_circuit_test_result = False
        self.capacitor_test_result = False
        self.communication_test_result = False
        self.calibration_test_result = False
        self.buck_test_result = False
        self.boost_test_result = False

        # ANSI escape codes for colors
        self.RED = '\033[91m'
        self.GREEN = '\033[92m'
        self.RESET = '\033[0m'

        self.leg_list = ["LEG1","LEG2"]

        self.meas_digits = 3

        self.default_data = {
            "V1": {"gain": 1.0, "offset": 0.0, "unit": 'V'},
            "V2": {"gain": 1.0, "offset": 0.0, "unit": 'V'},
            "VH": {"gain": 1.0, "offset": 0.0, "unit": 'V'},
            "I1": {"gain": 1.0, "offset": 0.0, "unit": 'A'},
            "I2": {"gain": 1.0, "offset": 0.0, "unit": 'A'},
            "IH": {"gain": 1.0, "offset": 0.0, "unit": 'A'}}

        self.calibration_test_results = {
            "V1": {"DMM": [], "TWIST": [], "gain": 1.0, "offset": 0.0, "r2": 0.0, "calibrated": False, "nvs": False},
            "V2": {"DMM": [], "TWIST": [], "gain": 1.0, "offset": 0.0, "r2": 0.0, "calibrated": False, "nvs": False},
            "VH": {"DMM": [], "TWIST": [], "gain": 1.0, "offset": 0.0, "r2": 0.0, "calibrated": False, "nvs": False},
            "I1": {"DMM": [], "TWIST": [], "gain": 1.0, "offset": 0.0, "r2": 0.0, "calibrated": False, "nvs": False},
            "I2": {"DMM": [], "TWIST": [], "gain": 1.0, "offset": 0.0, "r2": 0.0, "calibrated": False, "nvs": False},
            "IH": {"DMM": [], "TWIST": [], "gain": 1.0, "offset": 0.0, "r2": 0.0, "calibrated": False, "nvs": False}}

        self.cycling_test_results = {
            "TIME": {"TWIST": []},
            "T1": {"TWIST": []},
            "T2": {"TWIST": []}}

        self.step_response = {
            "LEG1": {"S_TIME": [], "OVER": [], "V_PEAK": [],"V_SET": [], "DELTA_I" : []},
            "LEG2": { }}

        self.efficiency_TWIST_results = {
            "LEG1": {"V1": [], "I1": [], "VH": [],"IH": []},
            "LEG2": {"V2": [], "I2": [], "VH": [],"IH": []}}

        self.efficiency_DMM_results = {
            "LEG1": {"VL": [],"IL": [],"VH": [],"IH": []},
            "LEG2": {"VL": [],"IL": [],"VH": [],"IH": []}}

        self.efficiency_duty = {
            "LEG1": {"duty": []},
            "LEG2": {"duty": []}}

        self.nvs_success_message = "Check for NVS writing (0 if correct) :0"
        self.nvs_error_message = "Error writing parameters in permanent storage!"

        #sets time delays in seconds
        self.short_delay = 0.2
        self.medium_delay = 1
        self.long_delay = 5

        # Creates a file to save all the test results
        self.Dir = os.path.dirname(os.path.abspath(__file__))
        self.Dir = os.path.join(self.Dir, "Results")
        if not os.path.exists(self.Dir):
            os.makedirs(self.Dir)
        self.CSVPathTest = self.Dir + '/' + 'factoryTests' + '_' 

        ResourceManager = pyvisa.ResourceManager()

        self.IHigh = K2000.KEITHLEY2000(ResourceManager.open_resource(FrontBackNoJsonSave.IHighGPIBAddress))
        self.VHigh = K2000.KEITHLEY2000(ResourceManager.open_resource(FrontBackNoJsonSave.VHighGPIBAddress))
        self.VLow = K2000.KEITHLEY2000(ResourceManager.open_resource(FrontBackNoJsonSave.VLowGPIBAddress))
        self.ILow = K2000.KEITHLEY2000(ResourceManager.open_resource(FrontBackNoJsonSave.ILowGPIBAddress))

        Twist_vid = 0x2fe3
        Twist_pid = 0x0100
        master_pid = 0x0101
        self.f = None
        self.state = IDLE
        self.buffer = ""
        self.f = io.StringIO()

        self.Fan_port = common.find_device(Twist_vid, master_pid)
        self.DUT_port = common.find_device(Twist_vid, Twist_pid)
        self.program_bin = "bins/DUT_bin_ayoub_v0.3.bin"

        #TODO: move this in the factory test script. Tried, line 157 needs to happend afterwards
        #self.ret_prog = prog_utils.flash_prog_bootloader(self.program_bin, self.DUT_port)
        # self.ret_prog = prog_utils.flash_prog_procedure(self.program_bin, self.DUT_port, "6217a66b9b2c96ddc99d50c9c0921e148c14b1d53a4a04f6e8303f7b1ebb92af")      
        # self.Fan_port = prog_utils.master_reset(self.Fan_port, Twist_vid, master_pid)

        # self.test_flags["TWIST"]["program"] = True
        # time.sleep(2)
        # self.DUT_port = common.find_device(Twist_vid, Twist_pid)
        

        self.PowerSupply = EAPSI9750.EAPSI9750(ResourceManager.open_resource(FrontBackNoJsonSave.PSUCommPort))
        self.ActiveLoadSerialObj = serial.Serial(FrontBackNoJsonSave.ActiveLoadCommPort)

        self.recorded_data_DUT = RecordedDatas.RecordedDatas(self.DUT_port, baudrate=115200, message_size= self.message_length)
        # self.DUT = serial.Serial(port=self.DUT_port)
        self.DUT = self.recorded_data_DUT.serial_port
        self.CooldownFanObj = serial.Serial(port=self.Fan_port)

        # # Init serial objects
        SPIN_Comm.InitSerialObjectTimeout(self.DUT, FrontBackNoJsonSave.STLINKbaudRate,
                                        FrontBackNoJsonSave.STLINKbytesize,FrontBackNoJsonSave.STLINKparity,
                                        FrontBackNoJsonSave.STLINKstopbits, FrontBackNoJsonSave.STLINKTimeout_s
        )#initializes the twist under test

        SPIN_Comm.InitSerialObjectTimeout(
            self.CooldownFanObj, FrontBackNoJsonSave.CooldownFanbaudRate,
            FrontBackNoJsonSave.CooldownFanbytesize,FrontBackNoJsonSave.CooldownFanparity,
            FrontBackNoJsonSave.CooldownFanstopbits, FrontBackNoJsonSave.CooldownFanTimeout_s
        ) #initializes the twist controlling the cooldown fan

        self.ActiveLoad_settings = {
            "Max": {"Voltage": 90, "Current": 6.0, "Power": 200}
        }
        self.ActiveLoad = LoadControl.IT8512BLoadController(self.ActiveLoadSerialObj,
                                                            FrontBackNoJsonSave.ActiveLoadAddress,
                                                            FrontBackNoJsonSave.ActiveLoadbaudRate,
                                                            FrontBackNoJsonSave.ActiveLoadparity,
                                                            FrontBackNoJsonSave.ActiveLoadsTimeout)
        self.ActiveLoad.set_remote(True)
        self.ActiveLoad.set_safety_settings(self.ActiveLoad_settings["Max"]["Voltage"],
                                       self.ActiveLoad_settings["Max"]["Current"],
                                       self.ActiveLoad_settings["Max"]["Power"])
        
        self.PowerSupply.init()


        if TempMeas is True :
            self.TempMeasObj = serial.Serial(FrontBackNoJsonSave.TempMeasCommPort)
            TempMeas.Init(self.TempMeasObj, FrontBackNoJsonSave.TempMeasbaudRate,
                        FrontBackNoJsonSave.TempMeasparity,
                        FrontBackNoJsonSave.TempMeasTimeout
            ) #initializes the temperature measurement device

    # def verif_serial(self, s):
    #     return self.boardSN.startswith(("TWT", "SPN"))

    # def read_qr(self):
    #     global frame

    #     camera_id = 0
    #     delay = 1
    #     window_name = 'OpenCV QR Code'

    #     qcd = cv2.QRCodeDetector()
    #     cap = cv2.VideoCapture(camera_id)

    #     cv2.namedWindow(window_name)
    #     #cv2.setMouseCallback(window_name, on_mouse_click)

    #     while True and self.boardSN == "" :
    #         ret, frame = cap.read()

    #         if ret:
    #             ret_qr, decoded_info, points, _ = qcd.detectAndDecodeMulti(frame)
    #             if ret_qr:
    #                 for s, p in zip(decoded_info, points):
    #                     if s:
    #                         self.boardSN = s
    #                         color = (0, 255, 0)
    #                         if (self.verif_serial == True):
    #                             print("read ", s)
                                 
                                
    #                     else:
    #                         color = (0, 0, 255)
    #                     frame = cv2.polylines(frame, [p.astype(int)], True, color, 8)


    #             cv2.imshow(window_name, frame)

    #         if cv2.waitKey(delay) & 0xFF == ord('q'):
    #             break
    #     cv2.destroyWindow(window_name)



        


    # def upload_test_report(self):
       
        
    #     with open(self.CSVPathTest, 'r') as ft:
    #         self.test_report = ft.read()
            


    #     img_reg_sum = self.boardSN + "_regression_summary.png"
    #     img_feeder_test = self.boardSN + "_regression_summary.png"

    #     prod_sdk.add_test_report(self.boardSN, self.test_report)


    # def get_test_report(self):
        
    #     self.CSVPathTest = self.Dir + '/' + 'factoryTests' + '_' + self.boardSN + '.csv'
        
    #     tests_json = prod_sdk.get_test_report(self.boardSN)
    #     report_txt = ""
    #     for k in range(len(tests_json["content"])):
    #         report_txt += tests_json["content"][k]['report']
    #     report_list = report_txt.split("\n")
    #     report_fin = []
        
    #     for id, row in enumerate(report_list) : 
    #         if "\t" in row : 
    #             report_fin += [row.split("\t")]

    #         else : 
    #             report_fin += [row.split(",")]
    #     self.report_fin = report_fin 
    #     with open(self.CSVPathTest, mode='w', newline='') as file:
    #             writer = csv.writer(file)
    #             writer.writerows(report_fin)
    
    # def clear_csv(self) :
    #     f = open(self.CSVPathTest, "w+")
    #     f.close()


    # def get_test_number(self) : 
    #     with open(self.CSVPathTest, 'r') as ft:
    #         reader = csv.reader(ft, delimiter = ',')
    #         for row in reader : 
    #             if row != [] : 
    #                 if row[0] == "Test number : " : 
    #                     self.testNumber = int(float(row[1]))
    #                     return 
    #     self.testNumber = 0

    




    def all_off(self) :
        """
        Turn off all devices.

        This method turns off the power supply, active load, and puts the DUT in IDLE mode.

        Returns:
            None
        """
        self.PowerSupply.powerOFF()
        self.ActiveLoad.enable_input(False)
        # SPIN_Comm.SendCommand(self.DUT,"IDLE")
        # SPIN_Comm.SendCommand(self.CooldownFanObj,"IDLE")

    # def test_calibrate(self):

    #     self.DUT.reset_input_buffer()
    #     SPIN_Comm.SendCommand(self.DUT, "CALIBRATE", 'V1', self.default_data['V1']["gain"], self.default_data['V1']["offset"])
        
    #     nvs_test_result = None
    #     nvs_check = False

    #     # Read lines until the line containing "Check for NVS writing" is found
    #     while nvs_check == False:
    #         line = self.DUT.readline().decode('utf-8').strip()
    #         if "Check for NVS writing" in line: 
    #             nvs_test_result = line.split(':')[-1].strip()  # Extract the NVS test result
    #             break  # Exit the loop when the line is found

    #     # Check if NVS test passed
    #     if nvs_test_result == "0!": NVS_test = True
    #     NVS_test = False



    #     # Print calibration and NVS status
    #     print("Calibration and NVS Status:")
    #     calibration_result = self.GREEN + "Success" + self.RESET if NVS_test else self.RED + "Failure" + self.RESET
    #     nvs_result = self.GREEN + "Success" + self.RESET if NVS_test else self.RED + "Failure" + self.RESET
    #     print(f"V1: Calibration - {calibration_result}, NVS - {nvs_result}")  

    # def perform_regression(self):
        
    #     fig, axs = plt.subplots(2, 3, figsize=(12, 8))
        
    #     # Perform linear regression and plot for each pair of arrays
    #     for i, (variable_name, data) in enumerate(self.calibration_test_results.items()):
    #         # Create DMM_data by adding random noise to TWIST_data
    #         TWIST_data = np.array(data["TWIST"])
    #         DMM_data = np.array(data["DMM"])
            
    #         # Perform linear regression
    #         gain, offset, r_value, p_value, std_err = stats.linregress(TWIST_data, DMM_data)
            
    #         if np.square(r_value) > 0.9 :
    #             self.calibration_test_results[variable_name]["gain"] = gain
    #             self.calibration_test_results[variable_name]["offset"] = offset
    #             self.calibration_test_results[variable_name]["r2"] = np.square(r_value)
    #             self.calibration_test_results[variable_name]["calibrated"] = True

    #             self.DUT.reset_input_buffer()
    #             SPIN_Comm.SendCommand(self.DUT, "CALIBRATE", variable_name, gain, offset)

    #             nvs_test_result = None

    #             # Read lines until the line containing "Check for NVS writing" is found
    #             while True:
    #                 line = self.DUT.readline().decode('utf-8').strip()
    #                 if "Check for NVS writing" in line: 
    #                     nvs_test_result = line.split(':')[-1].strip()  # Extract the NVS test result
    #                     break  # Exit the loop when the line is found

    #             # Check if NVS test passed
    #             if nvs_test_result == "0!": self.calibration_test_results[variable_name]["nvs"] = True

    #         else :
    #             self.calibration_test_results[variable_name]["calibrated"] = False


    #         # Plot the data and regression curve in the subplot
    #         row = i // 3
    #         col = i % 3
    #         axs[row, col].scatter(TWIST_data, DMM_data, label='Data')
    #         axs[row, col].plot(TWIST_data, TWIST_data * gain + offset, color='red', label='Regression Curve')
    #         axs[row, col].set_xlabel('TWIST Data (raw)', fontsize = 8)
    #         axs[row, col].set_ylabel(f'DMM Data ({self.default_data[variable_name]["unit"]})', fontsize = 8)
    #         axs[row, col].set_title(f'Regression Curve for {variable_name}', fontsize = 6)
    #         axs[row, col].legend(loc = 'lower right')
    #         axs[row, col].grid(True)

    #         # Add annotations for regression parameters
    #         axs[row, col].text(0.1, 0.9, f'Gain: {gain:.5f}', transform=axs[row, col].transAxes, fontsize=8,  verticalalignment='bottom')
    #         axs[row, col].text(0.1, 0.8, f'Offset: {offset:.3f}', transform=axs[row, col].transAxes, fontsize=8, verticalalignment='bottom')
    #         axs[row, col].text(0.1, 0.7, f'R-squared: {r_value**2:.2f}', transform=axs[row, col].transAxes, fontsize=8, verticalalignment='bottom')


    #     # Adjust layout
    #     plt.tight_layout(pad=0.4, w_pad=0.5, h_pad=1.0)
        
    #     # Show the plot     
    #     plt.show(block=False)
    #     # Close the plot window after 10 seconds
    #     plt.pause(10)
    #     plt.close()


    # def power_supply_check(self,init_value, end_value, n_steps):
    #     """
    #     Perform a power supply check.

    #     This method checks the power supply by varying the voltage from an initial value to an end value in steps and then back down.

    #     Args:
    #         init_value (float): Initial voltage value.
    #         end_value (float): End voltage value.
    #         n_steps (int): Number of steps.

    #     Returns:
    #         None
    #     """
    #     self.PowerSupply.setCurrentLimit_A(1.0)
    #     self.PowerSupply.setVoltage_V(init_value)
    #     self.PowerSupply.powerON()
    #     feeder_voltages_up = np.linspace(init_value, end_value, n_steps)
    #     feeder_voltages_down = np.linspace(end_value,init_value, n_steps)

    #     for test_voltage in feeder_voltages_up:
    #         self.PowerSupply.setVoltage_V(test_voltage)

    #     for test_voltage in feeder_voltages_down:
    #         self.PowerSupply.setVoltage_V(test_voltage)

    #     self.PowerSupply.powerOFF()  #need to implement a stop procedure for the power supply

    # def activeLoad_check(self,init_value, end_value, n_steps, mode):
    #     """
    #     Perform an active load check.

    #     This method checks the active load by varying the load from an initial value to an end value in steps and then back down.

    #     Args:
    #         init_value (float): Initial load value.
    #         end_value (float): End load value.
    #         n_steps (int): Number of steps.
    #         mode (str): Mode of the active load ('CC' for constant current, 'CV' for constant voltage, 'CR' for constant resistance).

    #     Returns:
    #         None
    #     """

    #     self.ActiveLoad.set_mode(mode)
    #     self.ActiveLoad.enable_input(True)
    #     active_load_up = np.linspace(init_value, end_value, n_steps)
    #     active_load_down = np.linspace(end_value,init_value, n_steps)

    #     for test_variable in active_load_up:
    #         if mode == 'CC': self.ActiveLoad.set_current_A(test_variable)
    #         if mode == 'CV': self.ActiveLoad.set_voltage_V(test_variable)
    #         if mode == 'CR': self.ActiveLoad.set_resistance_Ohm(test_variable)
    #         time.sleep(1)

    #     for test_variable in active_load_down:
    #         if mode == 'CC': self.ActiveLoad.set_current_A(test_variable)
    #         if mode == 'CV': self.ActiveLoad.set_voltage_V(test_variable)
    #         if mode == 'CR': self.ActiveLoad.set_resistance_Ohm(test_variable)
    #         time.sleep(1)

    #     self.ActiveLoad.enable_input(False)

    # def master_comm_check(self):
    #     #Master communication and cooldown fan Twist setup
    #     Dataset = []
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"IDLE")  #turns POWER OFF to retrieve communication test results
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"POWER_ON")  #turns POWER OFF to retrieve communication test results
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"POWER_OFF")  #turns POWER OFF to retrieve communication test results
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")  #turns POWER OFF to retrieve communication test results

    #     self.CooldownFanObj.reset_input_buffer()
    #     while True:  
    #         reading = self.CooldownFanObj.readline().decode('utf-8').split(':') 
    #         if len(reading) == 7 : break
        
    #     result_values = reading[0]
    #     result_values = result_values.strip("{}").split(",")
    #     RS485_success, Sync_success, Analog_success, Can_success = [bool(int(val)) for val in result_values]

    #     # Append individual test results to the dataset
    #     Dataset.append(("RS485", "Success" if RS485_success else "Failure"))
    #     Dataset.append(("Sync", "Success" if Sync_success else "Failure"))
    #     Dataset.append(("Analog", "Success" if Analog_success else "Failure"))
    #     Dataset.append(("Can", "Success" if Can_success else "Failure"))

    #     # Print calibration and NVS status
    #     RS485_result = self.GREEN + "Success" + self.RESET if RS485_success else self.RED + "Failure" + self.RESET
    #     Sync_result = self.GREEN + "Success" + self.RESET if Sync_success else self.RED + "Failure" + self.RESET
    #     Analog_result = self.GREEN + "Success" + self.RESET if RS485_success else self.RED + "Failure" + self.RESET
    #     Can_result = self.GREEN + "Success" + self.RESET if Can_success else self.RED + "Failure" + self.RESET

    #     print(f"RS485: Master Rest Test - {RS485_result}")  
    #     print(f"Sync: Master Read Test - {Sync_result}")  
    #     print(f"Analog: Master Rest Test - {Analog_result}")  
    #     print(f"CAN Bus: Master Read Test - {Can_result}")  

    #     # Write the last four variables from communication test result
    #     variables = self.get_TWIST_avg_measurement('AN')

    #     print(f"Analog: Slave Meas - {variables}")  


    # def TWIST_check(self,init_value, end_value, n_steps, active_load_mode):
    #     """
    #     Perform an active load check.

    #     This method checks the active load by varying the load from an initial value to an end value in steps and then back down.

    #     Args:
    #         init_value (float): Initial load value.
    #         end_value (float): End load value.
    #         n_steps (int): Number of steps.
    #         mode (str): Mode of the active load ('CC' for constant current, 'CV' for constant voltage, 'CR' for constant resistance).

    #     Returns:
    #         None
    #     """
    #     self.PowerSupply.setCurrentLimit_A(6.0)
    #     self.PowerSupply.setVoltage_V(20)
    #     self.PowerSupply.powerON()

    #     self.ActiveLoad.set_mode(active_load_mode)
    #     self.ActiveLoad.enable_input(True)
    #     active_load_up = np.linspace(init_value, end_value, n_steps)
    #     active_load_down = np.linspace(end_value,init_value, n_steps)

    #     Duty_values_up = np.linspace(0.1,0.5,10)
    #     Duty_values_down = np.linspace(0.5,0.1,10)

    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #     for test_variable in active_load_up:
    #         if active_load_mode == 'CC': self.ActiveLoad.set_current_A(test_variable)
    #         if active_load_mode == 'CV': self.ActiveLoad.set_voltage_V(test_variable)
    #         if active_load_mode == 'CR': self.ActiveLoad.set_resistance_Ohm(test_variable)
 
    #         for duty_up in Duty_values_up:
    #             SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", duty_up)

    #         for i in range(30): print(self.DUT.readline().decode('utf-8').split(':'))

    #         for duty_down in Duty_values_down:
    #             SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", duty_down)

    #     for test_variable in active_load_down:
    #         if active_load_mode == 'CC': self.ActiveLoad.set_current_A(test_variable)
    #         if active_load_mode == 'CV': self.ActiveLoad.set_voltage_V(test_variable)
    #         if active_load_mode == 'CR': self.ActiveLoad.set_resistance_Ohm(test_variable)
 
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #     self.ActiveLoad.enable_input(False)
    #     self.PowerSupply.powerOFF()

    # def DMM_measurement_check(self):
    #     """
    #     Check DMM measurements for voltage and current.

    #     This function retrieves voltage and current measurements from the digital multimeter (DMM)
    #     for the high and low side of the circuit. It prints the measurements of VHigh_V (high-side voltage),
    #     VLow_V (low-side voltage), IHigh_A (high-side current), and ILow_A (low-side current) to the console.

    #     Args:
    #         None

    #     Returns:
    #         None
    #     """

    #     IHigh_A = self.get_DMM_measurement('IH')
    #     VHigh_V = self.get_DMM_measurement('VH')
    #     VLow_V = self.get_DMM_measurement('VL')
    #     ILow_A = self.get_DMM_measurement('IL')

    #     print("VHigh_V:", VHigh_V)
    #     print("VLow_V:", VLow_V)
    #     print("IHigh_A:", IHigh_A)
    #     print("ILow_A:", ILow_A)

    # def TWIST_measurement_check(self):
    #     """
    #     Check TWIST measurements for voltage and current.

    #     This function retrieves voltage and current measurements from the TWIST system for both legs of the circuit.
    #     It sends commands to the device under test (DUT) to turn on power, retrieves measurements, and then sets the DUT
    #     to idle mode. It prints the measurements of VHigh_V (high-side voltage), VLow1_V (low-side voltage of leg 1),
    #     VLow2_V (low-side voltage of leg 2), IHigh_A (high-side current), ILow1_A (low-side current of leg 1),
    #     and ILow2_A (low-side current of leg 2) to the console.

    #     Args:
    #         None

    #     Returns:
    #         None
    #     """
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")              #turns POWER ON to retrive communication variables

    #     IHigh_A = self.get_TWIST_measurement('IH')
    #     VHigh_V = self.get_TWIST_measurement('VH')
    #     VLow1_V = self.get_TWIST_measurement('V1')
    #     ILow1_A = self.get_TWIST_measurement('I1')
    #     VLow2_V = self.get_TWIST_measurement('V2')
    #     ILow2_A = self.get_TWIST_measurement('I2')

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")              #turns POWER ON to retrive communication variables

    #     print("VHigh_V:", VHigh_V)
    #     print("VLow1_V:", VLow1_V)
    #     print("VLow2_V:", VLow2_V)
    #     print("IHigh_A:", IHigh_A)
    #     print("ILow1_A:", ILow1_A)
    #     print("ILow1_A:", ILow2_A)


    def get_DMM_measurement(self, measurement_type):
        """
        Function: get_measurement

        Description:
            Retrieves the desired DMM measurement value based on the measurement type.

        Parameters:
            - self: Instance of the class containing the measurement methods.
            - measurement_type (str): Type of measurement to retrieve. Supported types are 'VH' (Voltage High),
                                    'IH' (Current High), 'VL' (Voltage Low), and 'IL' (Current Low).

        Returns:
            - Measurement value corresponding to the specified measurement type.

        Raises:
            - ValueError: If an invalid measurement type is provided.

        """
        if measurement_type == 'VH':
            return self.VHigh.getAveragedVoltage_V(ReadOnlySettings.Number_of_DMM_READ_AVG,
                                                    ReadOnlySettings.DMM_READING_INTERVAL_ms)
        elif measurement_type == 'IH':
            return Power.ShuntVoltage_mVToCurr_A(self.IHigh.getAveragedVoltage_mV(ReadOnlySettings.Number_of_DMM_READ_AVG,
                                                                                ReadOnlySettings.DMM_READING_INTERVAL_ms),
                                                ReadOnlySettings.shunt_value_in_mOhm,
                                                ReadOnlySettings.IHighShuntCorrectionFactor)
        elif measurement_type == 'VL':
            return self.VLow.getAveragedVoltage_V(ReadOnlySettings.Number_of_DMM_READ_AVG,
                                                    ReadOnlySettings.DMM_READING_INTERVAL_ms)
        elif measurement_type == 'IL':
            return Power.ShuntVoltage_mVToCurr_A(self.ILow.getAveragedVoltage_mV(ReadOnlySettings.Number_of_DMM_READ_AVG,
                                                                                ReadOnlySettings.DMM_READING_INTERVAL_ms),
                                                ReadOnlySettings.shunt_value_in_mOhm,
                                                ReadOnlySettings.ILowShuntCorrectionFactor)
        else:
            raise ValueError("Invalid measurement type. Please use one of: 'VH', 'IH', 'VL', 'IL'")

    def get_TWIST_measurement(self, measurement_type):
        """
        Retrieves the TWIST measurement value based on the measurement type.
        This MUST be called when the Twist is in POWER ON

        Parameters:
            - self: Instance of the class containing the measurement methods.
            - measurement_type (str): Type of measurement to retrieve.
              Supported types are 'V1', 'V2', 'VH', 'I1', 'I2', 'IH', 'M1', 'M2','CT' , 'T1', 'T2'.

        Returns:
            - Measurement value corresponding to the specified measurement type.

        Raises:
            - ValueError: If an invalid measurement type is provided.
        """
        if measurement_type not in self.DUT_message_index:
            raise ValueError("Invalid measurement type. Supported types are 'V1', 'V2', 'T1', 'T2', 'VH', 'I1', 'I2', 'IH', 'M1', 'M2','CT'.")

        # time.sleep(self.short_delay)
        index_meas = self.DUT_message_index[measurement_type]["index"]

        self.DUT.reset_input_buffer()

        size_check = False
        while size_check == False:  
            reading = self.DUT.readline().decode('utf-8').split(':')
            reading = [elem.replace('{', '').replace('}', '') for elem in reading] #eliminates curly brackets from the message
            if len(reading) == self.message_length : size_check = True               #14 is the length of the communication buffer

        return float(reading[index_meas])


    # def program_DUT(self):
    #     """
    #     Program the DUT with a testing program.
    #     """

    #     CSVPathTest2 = self.Dir + '/' + 'factoryTests' + '_' + self.boardSN + '_num' + str(self.testNumber) + '.csv'
    #     if not os.path.exists(CSVPathTest2):
    #         os.rename(self.CSVPathTest, CSVPathTest2)
    #     self.CSVPathTest = CSVPathTest2

    #     print("----------------------")
    #     print("Start Programming DUT")
        
    #     #ret_prog = prog_utils.flash_prog_bootloader(self.program_bin, self.DUT_port)
    #     #TODO: Fix me! Can't work bc the DUT serial port is already opened
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter='\t')
    #         writer.writerow(["Serial number", self.boardSN])
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Program", "Test"])
    #         writer.writerow(["Test number : ", self.testNumber])
    #         writer.writerow(["Date", str(datetime.datetime.now())])
    #         writer.writerow(["Bin", self.program_bin])
    #         writer.writerow(["Result", self.ret_prog[1]])
    #         writer.writerow(["------", "------"])
    #         #Average speed ??
    #     self.test_flags["TWIST"]["program"] = self.ret_prog[2]
    
    # def feeder_test(self):
    #     """
    #     Perform the feeder test.

    #     This method initializes and executes the feeder test. It measures the current and voltage at various feeder voltages and saves the data to a CSV file.

    #     Returns:
    #         None
    #     """
    #     print("----------------------")
    #     print("Start Feeder Test")

    #     Dataset = []
    #     self.PowerSupply.setCurrentLimit_A(0.3)
    #     self.PowerSupply.powerON()
    #     min_v = 15
    #     max_v = 85
    #     steps = 10

    #     feeder_voltages_up = np.linspace(min_v, max_v, steps)
    #     feeder_voltages_down = np.linspace(max_v, min_v, steps)

    #     for count, test_voltage in enumerate(feeder_voltages_up, start=1):
    #         self.PowerSupply.setVoltage_V(test_voltage)
    #         time.sleep(self.short_delay)
    #         IHigh_A = self.get_DMM_measurement('IH')
    #         VHigh_V = self.get_DMM_measurement('VH')
    #         Dataset.append((IHigh_A, VHigh_V))
    #         progress_bar.progress_bar(count,steps,prefix='Feeder test:', suffix='Complete', length=40)

    #     for test_voltage in feeder_voltages_down:
    #         self.PowerSupply.setVoltage_V(test_voltage)
    #         time.sleep(self.short_delay)

    #     # Separate the IHigh_A and VHigh_V values from the Dataset
    #     IHigh_A_values = [data[0] for data in Dataset]
    #     VHigh_V_values = [data[1] for data in Dataset]        

    #     # Plot the results
    #     plt.scatter(VHigh_V_values, IHigh_A_values)
    #     # Ideal curve points
    #     y_ideal = np.array([0.165, 0.108, 0.082, 0.066, 0.056, 0.0486, 0.0434, 0.0395, 0.0365, 0.0344])
    #     x_ideal = np.array([15.043, 22.829, 30.603, 38.388, 46.149, 53.928, 61.699, 69.490, 77.273, 85.039])
        
    #     # Define a corridor of acceptable values (you can define your own corridor)
    #     corridor_percent = 20  # percent corridor
    #     spline_interp = interp1d(x_ideal, y_ideal, kind='cubic') # Perform spline interpolation


    #     # Generate x values for the smoothed curve
    #     x_smooth = np.linspace(min(x_ideal), max(x_ideal), 1000)

    #     # Calculate y values using spline interpolation
    #     y_smooth = spline_interp(x_smooth)

    #     lower_bound = y_smooth - (y_smooth*corridor_percent/100)
    #     upper_bound = y_smooth + (y_smooth*corridor_percent/100)

    #     plt.plot(x_smooth, y_smooth, color='red', label='Ideal Curve')
    #     # Fill between the corridor
    #     plt.fill_between(x_smooth, lower_bound, upper_bound, color='gray', alpha=0.3, label='Acceptable Range')
        
    #     plt.xlabel('Feeder Input Voltage (V)', fontsize = 8)
    #     plt.ylabel('Feeder Input Current (A)', fontsize = 8)
    #     plt.title('Feeder Test Results', fontsize = 8)
    #     plt.grid(True)

    #     # Show the plot     
    #     plt.show(block=False)

    #     # Close the plot window after 10 seconds
    #     plt.pause(5)
    #     plt.close()


    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter='\t')
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Feeder", "Test"])
    #         writer.writerow(["IHigh_A", "VHigh_V"])
    #         writer.writerows(Dataset)
    #         writer.writerow(["------", "------"])

    #     #Calculate if the measurements are within the accepted range
    #     self.PowerSupply.powerOFF()  #need to implement a stop procedure for the power supply
    #     self.PowerSupply.setCurrentLimit_A(1.0)

    #     # Perform feeder test
    #     # Update test result
    #     #Test if there are any zeros on the Vhigh or Ihigh measurements (this means the source has protected itself)
    #     self.test_flags["TWIST"]["feeder"] = all(value != 0 for value in IHigh_A_values) and \
    #                                          all(value != 0 for value in VHigh_V_values)
    #     for x, y in zip(VHigh_V_values, IHigh_A_values):
    #         if not np.any((y >= lower_bound) & (y <= upper_bound)):
    #             self.test_flags["TWIST"]["feeder"] = False
    #             return
    #     self.test_flags["TWIST"]["feeder"] = True

    # def communication_test(self):
    #     """
    #     Performs the communication test.

    #     This function initializes the communication test, waits for a specified time,
    #     reads the communication test result line and parses it to extract the values
    #     of RS485_success, Sync_success, Analog_success, and Can_success. It appends
    #     individual test results along with their corresponding references to the dataset.
    #     Additionally, it reads the last four variables from the communication test result,
    #     appends them to the dataset, and writes the dataset to a CSV file.

    #     Parameters:
    #         self (object): The object instance.

    #     Returns:
    #         None
    #     """
    #     print("----------------------")
    #     print("Start Communication Test")
    #     # Initialize the dataset
    #     Dataset = []
    #     Vhigh_value = 20
    #     Ihigh_limit = 1.0
        
    #     #Power supply setup
    #     self.PowerSupply.setVoltage_V(Vhigh_value)                          #sets the power supply voltage to 20V
    #     self.PowerSupply.setCurrentLimit_A(Ihigh_limit)                    #sets the power supply current limit to 1A
    #     self.PowerSupply.powerON()                                 #turns the power supply ON

    #     self.ActiveLoad.enable_input(False)

    #     #DUT setup
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","OFF")      #makes sure the DUT cannot switch
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","OFF")   #makes sure the DUT LEG1 is not on
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","OFF")      #makes sure the DUT cannot switch
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG2","OFF")   #makes sure the DUT LEG1 is not on
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"IDLE")  #turns POWER OFF to retrieve communication test results
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"POWER_ON")  #turns POWER OFF to retrieve communication test results
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"POWER_OFF")  #turns POWER OFF to retrieve communication test results
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON",delay=self.medium_delay)  #turns POWER OFF to retrieve communication test results

    #     self.CooldownFanObj.reset_input_buffer()
    #     while True:  
    #         reading = self.CooldownFanObj.readline().decode('utf-8').split(':') 
    #         if len(reading) == 7 : break
        
    #     result_values = reading[0]
    #     result_values = result_values.strip("{}").split(",")
    #     RS485_success, Sync_success, Analog_success, Can_success = [bool(int(val)) for val in result_values]

    #     # Append individual test results to the dataset
    #     Dataset.append(("RS485", "Success" if RS485_success else "Failure"))
    #     Dataset.append(("Sync", "Success" if Sync_success else "Failure"))
    #     Dataset.append(("Analog", "Success" if Analog_success else "Failure"))
    #     Dataset.append(("Can", "Success" if Can_success else "Failure"))

    #     # Print calibration and NVS status
    #     RS485_result = self.GREEN + "Success" + self.RESET if RS485_success else self.RED + "Failure" + self.RESET
    #     Sync_result = self.GREEN + "Success" + self.RESET if Sync_success else self.RED + "Failure" + self.RESET
    #     Analog_result = self.GREEN + "Success" + self.RESET if RS485_success else self.RED + "Failure" + self.RESET
    #     Can_result = self.GREEN + "Success" + self.RESET if Can_success else self.RED + "Failure" + self.RESET

    #     print(f"RS485: Master Rest Test - {RS485_result}")  
    #     print(f"Sync: Master Read Test - {Sync_result}")  
    #     print(f"Analog: Master Rest Test - {Analog_result}")  
    #     print(f"CAN Bus: Master Read Test - {Can_result}")  

    #     self.test_flags["TWIST"]["RS485"] = RS485_success   
    #     self.test_flags["TWIST"]["Sync"] = Sync_success    
    #     self.test_flags["TWIST"]["Analog"] = Analog_success  
    #     self.test_flags["TWIST"]["CAN"] = Can_success       

    #     # Write the last four variables from communication test result
    #     variables = self.get_TWIST_avg_measurement('AN')
    #     print(f"Analog: Slave Meas - {variables} - reference 1000")  
    #     Dataset.append(("Slave Analog", f"{variables}"))
    #     Dataset.append(("Master Reference", "1000"))

    #     # Write the dataset to a CSV file
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter='\t')
    #         # Write the separator and test header
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Comm", "Test"])
    #         # Write individual test results
    #         writer.writerows(Dataset)
    #         # Write the closing separator
    #         writer.writerow(["------", "------"])

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")                  #sets DUT to IDLE
    #     SPIN_Comm.SendCommand(self.CooldownFanObj,"IDLE")       #sets Master Comm to IDLE
    #     self.PowerSupply.powerOFF()                             #turns the power supply OFF




    # def driver_switch_test(self):
    #     # Initialize the dataset
    #     print("----------------------")
    #     print("Start Driver Test")

    #     Dataset = []
    #     Vhigh_value = 20
    #     Ihigh_limit = 1.0

    #     #Power supply setup
    #     self.PowerSupply.setVoltage_V(Vhigh_value)                          #sets the power supply voltage to 20V
    #     self.PowerSupply.setCurrentLimit_A(Ihigh_limit)                    #sets the power supply current limit to 1A
    #     self.PowerSupply.powerON()                                #turns the power supply ON
    #     self.ActiveLoad.enable_input(False)

    #     #DUT setup
    #     SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1",0.5)
    #     SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG2",0.5)

    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #     VLow1_V = self.get_DMM_measurement("VL")

    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG2","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","ON")

    #     VLow2_V = self.get_DMM_measurement("VL")

    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG2","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")                  #sets DUT to IDLE

    #     Dataset.append(["LEG1", VLow1_V, "Success" if VLow1_V > 9.0  else "Failure"])
    #     Dataset.append(["LEG2", VLow2_V, "Success" if VLow2_V > 9.0 else "Failure"])

    #     #print the results of the test
    #     leg1_result = self.GREEN + "Success" + self.RESET if VLow1_V > 9.0 else self.RED + "Failure" + self.RESET
    #     leg2_result = self.GREEN + "Success" + self.RESET if VLow2_V > 9.0 else self.RED + "Failure" + self.RESET
    #     print(f"LEG1: Driver - {leg1_result}")  
    #     print(f"LEG2: Driver - {leg2_result}")  

    #     # Write the dataset to a CSV file
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter='\t')
    #         # Write the separator and test header
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Driver", "Test"])
    #         # Write individual test results
    #         writer.writerows(Dataset)
    #         # Write the closing separator
    #         writer.writerow(["------", "------"])

    #     self.ActiveLoad.enable_input(False)
    #     self.PowerSupply.powerOFF()                             #turns the power supply OFF

    #     if VLow1_V > 9.0 : self.test_flags["LEG1"]["driver"] = True 
    #     if VLow2_V > 9.0 : self.test_flags["LEG2"]["driver"] = True 

    # def get_test_results_DUT(self, test_name: str) -> bool:
    #     """
    #     Get the test result for a specific test name.

    #     Parameters:
    #         test_name (str): The name of the test.

    #     Returns:
    #         bool: The result of the specified test.
    #         The valid test names are "program", "feeder", "RS485", "Sync", "Analog", "CAN". 
    #         True if passed, False if failed.
    #     """
    #     valid_test_names = ["program", "feeder", "RS485", "Sync", "Analog", "CAN"]
    #     if test_name in valid_test_names:
    #         return self.test_flags["TWIST"][test_name]
    #     else:
    #         raise ValueError("Invalid test name. Valid test names are: " + ", ".join(valid_test_names))

    # def get_test_results_DUT_LEG(self, leg: str, test_name: str) -> bool:
    #     """
    #     Get the test result for a specific test in the specified LEG.

    #     Parameters:
    #         leg (str): The name of the LEG ("LEG1" or "LEG2").
    #         test_name (str): The name of the test to check. Expected test names:
    #                         - "capa": Capacitor test
    #                         - "driver": Driver test
    #                         - "nvs": Non-volatile memory test
    #                         - "calib": Calibration test
    #                         - "buck": Buck converter test
    #                         - "boost": Boost converter test
    #                         - "open circuit": Open circuit test
    #                         - "efficiency": Efficiency test

    #     Returns:
    #         bool: True if the test is passed, False otherwise.
    #     """
    #     # Validate the input leg
    #     if leg not in self.leg_list:
    #         print(f"Error: Invalid LEG name '{leg}'.")
    #         return False

    #     # Validate the input test_name
    #     valid_test_names = ["capa", "driver", "nvs", "calib", "buck", "boost", "open circuit", "efficiency"]
    #     if test_name not in valid_test_names:
    #         print(f"Error: Invalid test name '{test_name}'. Expected test names: {', '.join(valid_test_names)}.")
    #         return False

    #     # Check if the test result exists and return it
    #     return self.test_flags[leg][test_name]

    # def open_circuit_test(self):
    #     """
    #     @brief Perform the open circuit test.

    #     This function initializes the dataset, sets up the power supply, and performs the open circuit test for each leg.
    #     For each leg, it ramps up the duty cycle, measures Vhigh and Vlow values, and stores them in a dictionary.
    #     The duty cycle, Vhigh, and Vlow values for each leg are then written to a CSV file.

    #     @return: None
    #     """
    #     print("----------------------")
    #     print("Start Open Circuit Test")

    #     Vhigh_value = 20
    #     Ihigh_limit = 1.0
    #     # Dictionaries to hold duty cycle, Vhigh, and Vlow values for each leg
    #     leg_dict = {"LEG1": {"duty": [], "IH": [], "VL": [], "r2":0.0},
    #                 "LEG2": {"duty": [], "IH": [], "VL": [], "r2":0.0}}

    #     #DUT LEG1 setup
    #     initial_duty = 0.1
    #     final_duty = 0.8
    #     duty_steps = 10
    #     duty_values_up = np.linspace(initial_duty, final_duty, duty_steps)
    #     duty_values_down = np.linspace(final_duty, initial_duty, duty_steps)

    #     #Power supply setup
    #     self.PowerSupply.setVoltage_V(Vhigh_value)                          #sets the power supply voltage to 20V
    #     self.PowerSupply.setCurrentLimit_A(Ihigh_limit)                    #sets the power supply current limit to 1A
    #     self.PowerSupply.powerON()                                 #turns the power supply ON

    #     for leg_to_test in self.leg_list:
    #         SPIN_Comm.SendCommand(self.DUT,"DRIVER",leg_to_test,"ON")
    #         SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"ON")
    #         SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #         for count, duty in enumerate(duty_values_up, start=1):
    #             SPIN_Comm.SendCommand(self.DUT,"DUTY",leg_to_test, duty)

    #             IHigh_A = self.get_DMM_measurement('IH')
    #             VLow_V = self.get_DMM_measurement('VL')

    #             # Store duty cycle, Vhigh, and Vlow values in the dictionary
    #             leg_dict[leg_to_test]["duty"].append(duty)
    #             leg_dict[leg_to_test]["IH"].append(IHigh_A)
    #             leg_dict[leg_to_test]["VL"].append(VLow_V)
    #             progress_bar.progress_bar(count,duty_steps,prefix=f'{leg_to_test} Open Circuit test:', suffix='Complete', length=40)

    #         for duty in duty_values_down :                                  #ramps the duty cycle down
    #             SPIN_Comm.SendCommand(self.DUT,"DUTY",leg_to_test, duty)

    #         SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"OFF")
    #         SPIN_Comm.SendCommand(self.DUT,"DRIVER",leg_to_test,"OFF")

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")                  #sets DUT to IDLE
    #     self.PowerSupply.powerOFF()                             #turns the power supply OFF
    #     #----------PLOT-------------------------------------------

    #     # Create a figure with subplots for LEG1 and LEG2
    #     fig, axs = plt.subplots(1, 2)

    #     # Plot LEG1
    #     axs[0].plot(leg_dict["LEG1"]["duty"], leg_dict["LEG1"]["VL"], label='IH LEG1', color='tab:blue')
    #     axs[0].set_xlabel('Duty')
    #     axs[0].set_ylabel('VLow1 (V)', color='tab:red')
    #     axs[0].tick_params(axis='y', labelcolor='tab:red')
    #     axs[0].legend()
    #     axs[0].grid(True)

    #     # Plot LEG2
    #     axs[1].plot(leg_dict["LEG2"]["duty"], leg_dict["LEG2"]["VL"], label='IH LEG2', color='tab:blue')
    #     axs[1].set_xlabel('Duty')
    #     axs[1].set_ylabel('VLow2 (V)', color='tab:green')
    #     axs[1].tick_params(axis='y', labelcolor='tab:green')
    #     axs[1].legend()
    #     axs[1].grid(True)

    #     plt.tight_layout()

    #     # Show the plot     
    #     plt.show(block=False)
    #     # Close the plot window after 10 seconds
    #     plt.pause(5)
    #     plt.close()


    #     # Write the dataset to a CSV file
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter='\t')
    #         # Write the separator and test header
    #         writer.writerow(["------", "------", "------"])
    #         writer.writerow(["open", "circuit", "Test"])
    #         # Write individual test results
    #         writer.writerow(["Duty", "IH_LEG1", "VL_LEG1", "IH_LEG2", "VL_LEG2"])
    #         # Write individual test results
    #         for i in range(len(duty_values_up)):
    #             writer.writerow([round(leg_dict["LEG1"]["duty"][i], self.meas_digits),
    #                             round(leg_dict["LEG1"]["IH"][i], self.meas_digits), round(leg_dict["LEG1"]["VL"][i], self.meas_digits),
    #                             round(leg_dict["LEG2"]["IH"][i], self.meas_digits), round(leg_dict["LEG2"]["VL"][i], self.meas_digits)])
    #         # Write the closing separator
    #         writer.writerow(["------", "------"])

    #     #tests if the duty and the VL are linear to one another. 
    #     #If this is true, then the open-circuit is working as expencted in Buck mode
    #     g, o, r, p_value, std_err = stats.linregress(np.array(leg_dict["LEG1"]["duty"]), np.array(leg_dict["LEG1"]["VL"]))
    #     self.test_flags["LEG1"]["open circuit"] = True if r*r > 0.99  else False
    #     g, o, r, p_value, std_err = stats.linregress(np.array(leg_dict["LEG2"]["duty"]), np.array(leg_dict["LEG2"]["VL"]))
    #     self.test_flags["LEG2"]["open circuit"] = True if r*r > 0.99  else False

    # def capacitor_test(self):
    #     """
    #     Perform a capacitor test for each leg.

    #     This function initializes the power supply, sets the voltage and current limit,
    #     and conducts a capacitor test for each leg. It reads the test data for each leg
    #     and writes the results to a CSV file. The results include the maximum voltage
    #     values during the capacitor test with the capacitor on (VC_ON) and off (VC_OFF)
    #     for each leg.

    #     Args:
    #         None

    #     Returns:
    #         None
    #     """
    #     print("----------------------")
    #     print("Start Capacitor Test")

    #     Vhigh_value = 20
    #     Ihigh_limit = 6.0

    #     #Power supply setup
    #     self.PowerSupply.setVoltage_V(Vhigh_value)                          #sets the power supply voltage to 20V
    #     self.PowerSupply.setCurrentLimit_A(Ihigh_limit)                    #sets the power supply current limit to 1A
    #     self.PowerSupply.powerON()                                 #turns the power supply ON

    #     self.ActiveLoad.set_mode('CR')
    #     self.ActiveLoad.set_resistance_Ohm(1000)
    #     self.ActiveLoad.enable_input(True)

    #     # Initialize dictionary to store test results for each leg
    #     capa_values = {"LEG1": {"V_max_C_ON": None, "V_max_C_OFF": None, "meas_type": 'M1'},
    #                    "LEG2": {"V_max_C_ON": None, "V_max_C_OFF": None, "meas_type": 'M2'}}

    #     #Test procedure
    #     for leg_to_test in self.leg_list:
    #         SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #         SPIN_Comm.SendCommand(self.DUT,"DUTY",leg_to_test, 0.5)
    #         SPIN_Comm.SendCommand(self.DUT,"POWER_ON")
    #         SPIN_Comm.SendCommand(self.DUT,"DRIVER",leg_to_test,"ON")
    #         SPIN_Comm.SendCommand(self.DUT,"CAPA",leg_to_test,"ON")
    #         SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"ON")

    #         capa_values[leg_to_test]["V_max_C_ON"] = self.get_TWIST_measurement(capa_values[leg_to_test]["meas_type"])

    #         SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"OFF")
    #         SPIN_Comm.SendCommand(self.DUT,"CAPA",leg_to_test,"OFF")
    #         SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"ON")

    #         capa_values[leg_to_test]["V_max_C_OFF"] = self.get_TWIST_measurement(capa_values[leg_to_test]["meas_type"])

    #         SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"OFF")
    #         SPIN_Comm.SendCommand(self.DUT,"CAPA",leg_to_test,"OFF")
    #         SPIN_Comm.SendCommand(self.DUT,"DRIVER",leg_to_test,"OFF")
    #         SPIN_Comm.SendCommand(self.DUT,"IDLE")

    #         # Print calibration and NVS status
    #         test_result = self.GREEN + "Success" + self.RESET if capa_values[leg_to_test]["V_max_C_OFF"] > capa_values[leg_to_test]["V_max_C_ON"] else self.RED + "Failure" + self.RESET
    #         print(f"{leg_to_test}: Capacitor Test Result - {test_result}")  



    #     # Write the dataset to a CSV file
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter='\t')
    #         # Write the separator and test header
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Capacitor", "Test"])
    #         writer.writerow(["LEG", "VC_ON", "VC_OFF"])

    #         # Write individual test results
    #         for leg, values in capa_values.items():
    #             writer.writerow([leg, values["V_max_C_ON"], values["V_max_C_OFF"]])            # Write the closing separator

    #         writer.writerow(["------", "------"])

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")                  #sets DUT to IDLE
    #     self.ActiveLoad.enable_input(False)
    #     self.PowerSupply.powerOFF()                             #turns the power supply OFF

    #     if capa_values["LEG1"]["V_max_C_OFF"] > capa_values["LEG1"]["V_max_C_ON"] :  self.test_flags["LEG1"]["capa"] = True
    #     if capa_values["LEG2"]["V_max_C_OFF"] > capa_values["LEG2"]["V_max_C_ON"] :  self.test_flags["LEG2"]["capa"] = True



    def get_TWIST_avg_measurement(self,measurement_type,number_of_measurements = 3):
        """
        Get the average measurement value for a specified type from the TWIST device.

        This method takes multiple measurements of the specified type from the TWIST device and returns the average value.

        Parameters:
            measurement_type (str): The type of measurement to be taken. Supported types are 'V1', 'V2', 'VH', 'I1', 'I2', 'T1', 'T2', 'IH', 'M1', 'M2', 'CT'.
            number_of_measurements (int): The number of measurements to be taken for averaging. Default is 3.

        Returns:
            float: The average measurement value.

        Raises:
            ValueError: If the measurement type is not supported.
        """

        if measurement_type not in self.DUT_message_index:
            raise ValueError("Invalid measurement type. Supported types are 'V1', 'V2', 'VH', 'I1', 'I2', 'T1', 'T2', 'IH', 'M1', 'M2','CT'.")

        meas_buffer = np.zeros(number_of_measurements)

        for i in range(number_of_measurements):
            meas_buffer[i] = self.get_TWIST_measurement(measurement_type)

        return np.mean(meas_buffer)




    # def calibration_test(self):
    #     """
    #     Perform the calibration test.

    #     This method performs the calibration test of each measurement embedded onto the TWIST board.
    #     It varies the VH, VL, IL and IH independently for each leg.
    #     It iterates through different settings, collects measurements, calculates coefficients using linear regression and
    #     saves the calibration coefficients to the non-volatile storage (NVS) of the TWIST board.
    #     The dataset and the calibration coefficients are saved to a CSV file.

    #     Parameters:
    #     None

    #     Returns:
    #     None
    #     """
    #     print("----------------------")
    #     print("Start Calibration Test")
    #     #Power supply setup
    #     Vhigh_value = 20
    #     Ihigh_limit = 6.0

    #     #Power supply setup
    #     self.PowerSupply.setVoltage_V(Vhigh_value)                          #sets the power supply voltage to 20V
    #     self.PowerSupply.setCurrentLimit_A(Ihigh_limit)                    #sets the power supply current limit to 1A
    #     self.PowerSupply.powerON()                                 #turns the power supply ON
    #     self.ActiveLoad.enable_input(False)
    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")

    #     min_duty = 0.1
    #     max_duty_I = 0.5
    #     max_duty_VL = 0.8   #orinal value = 0.8
    #     n_duty_I = 10
    #     steps_duty = 8       #original value = 8

    #     min_v = 10
    #     max_v = 80          #original value = 80
    #     steps_v = 8         #original value = 8

    #     min_i = 1
    #     max_i = 6.0         #original value = 6.0
    #     steps_i = 6         #original value = 6


    #     duty_sweep_up   = np.linspace(min_duty, max_duty_I, n_duty_I)
    #     duty_sweep_down = np.linspace(max_duty_I, min_duty, n_duty_I)

    #     VLow_duty_up    = np.linspace(min_duty, max_duty_VL, steps_duty)
    #     VLow_duty_down  = np.linspace(max_duty_VL, min_duty, steps_duty)


    #     VHigh_values_up = np.linspace(min_v, max_v, steps_v)
    #     VHigh_values_down = np.linspace(max_v, min_v, steps_v)

    #     ILow_values_up = np.linspace(min_i, max_i, steps_i)
    #     ILow_values_down = np.linspace(max_i, min_i, steps_i)

        

    #     for variable_name, variable_info in self.default_data.items():      #resets the DUT measurement parameters to a gain of 1 and an offset of 0
    #         SPIN_Comm.SendCommand(self.DUT, "CALIBRATE", variable_name, variable_info["gain"], variable_info["offset"])
        
    #     print("Calibration Test Setup Complete")
    #     print("Calibrating ..... ")


    #     #-----------------------VHIGH CALIBRATION-----------------------------------------------------------------
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #     for count, test_voltage in enumerate(VHigh_values_up, start=1):
    #         self.PowerSupply.setVoltage_V(test_voltage)
    #         time.sleep(self.short_delay)
    #         self.calibration_test_results["VH"]["DMM"].append(self.get_DMM_measurement('VH'))
    #         self.calibration_test_results["VH"]["TWIST"].append(self.get_TWIST_avg_measurement('VH'))
    #         progress_bar.progress_bar(count,steps_v,prefix='VH Calibration:', suffix='Complete', length=40)

    #     for sweep_down_voltage in VHigh_values_down:
    #         self.PowerSupply.setVoltage_V(sweep_down_voltage)
      
    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")

    #     #-----------------------CURRENT CALIBRATION-----------------------------------------------------------------
        
    #     self.ActiveLoad.set_mode('CC')
    #     self.ActiveLoad.set_current_A(0)
    #     self.ActiveLoad.enable_input(True) # maybe there should be a handshake or something...

    #     SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", min_duty)
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #     for duty_up in duty_sweep_up: SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", duty_up)  #ramps up the duty cycle

    #     for count, test_current in enumerate(ILow_values_up, start=1):
    #         self.ActiveLoad.set_current_A(test_current)
    #         time.sleep(0.2)

    #         self.calibration_test_results["I1"]["DMM"].append(self.get_DMM_measurement('IL'))
    #         self.calibration_test_results["I1"]["TWIST"].append(self.get_TWIST_avg_measurement('I1'))

    #         self.calibration_test_results["IH"]["DMM"].append(self.get_DMM_measurement('IH'))
    #         self.calibration_test_results["IH"]["TWIST"].append(self.get_TWIST_avg_measurement('IH'))
    #         progress_bar.progress_bar(count,steps_i,prefix='I1 and IH Calibration:', suffix='Complete', length=40)

    #     for current_down in ILow_values_down : self.ActiveLoad.set_current_A(current_down)  #ramps down the duty cycle
    #     for duty in duty_sweep_down: SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", duty)  #ramps down the duty cycle
    #     self.ActiveLoad.set_current_A(0)

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG2","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG2","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #     for duty in duty_sweep_up: SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG2", duty)  #ramps up the duty cycle

    #     for count, test_current in enumerate(ILow_values_up, start=1):
    #         self.ActiveLoad.set_current_A(test_current)
    #         time.sleep(0.2)

    #         self.calibration_test_results["I2"]["DMM"].append(self.get_DMM_measurement('IL'))
    #         self.calibration_test_results["I2"]["TWIST"].append(self.get_TWIST_avg_measurement('I2'))
    #         progress_bar.progress_bar(count,steps_i,prefix='I2 Calibration:', suffix='Complete', length=40)

    #     for duty in duty_sweep_down: SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG2", duty)  #ramps up the duty cycle

    #     self.ActiveLoad.set_current_A(0)
    #     self.ActiveLoad.enable_input(False) #turns the active load off

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG2","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG2","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG2","OFF")


    #     #-----------------------VLOW CALIBRATION-----------------------------------------------------------------


    #     for VHigh_value in VHigh_values_up : self.PowerSupply.setVoltage_V(VHigh_value)

    #     self.ActiveLoad.set_current_A(1)
    #     self.ActiveLoad.enable_input(True) #turns the active load on

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","ON")
    #     SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #     for count, duty_cycle in enumerate(VLow_duty_up, start=1):
    #         SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", duty_cycle)

    #         self.calibration_test_results["V1"]["DMM"].append(self.get_DMM_measurement('VL'))
    #         self.calibration_test_results["V2"]["DMM"].append(self.get_DMM_measurement('VL'))
    #         self.calibration_test_results["V1"]["TWIST"].append(self.get_TWIST_avg_measurement('V1'))
    #         self.calibration_test_results["V2"]["TWIST"].append(self.get_TWIST_avg_measurement('V2'))
    #         progress_bar.progress_bar(count,steps_duty,prefix='V1 and V2 Calibration:', suffix='Complete', length=40)

    #     for sweep_duty_down in VLow_duty_down: SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1", sweep_duty_down)

    #     for sweep_values_down in VHigh_values_down :    #ramps down the VHigh from 80V to 10V
    #         self.PowerSupply.setVoltage_V(sweep_values_down)
    #         time.sleep(self.short_delay)

    #     print("Calibration data Acquisition Complete")
    #     print("Power Shutdown")

    #     SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #     SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","OFF")
    #     SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","OFF")
    #     self.ActiveLoad.enable_input(False)
    #     self.PowerSupply.powerOFF()                             #turns the power supply OFF


    #     #-----------------------CALCULATES THE COEFFICIENTS-----------------------------------------------------------------

    #     self.perform_regression()

    #     print("Coefficients calculation Results ")
    #     print("-------------------------------- ")

    #     # ANSI escape codes for colors
    #     RED = '\033[91m'
    #     GREEN = '\033[92m'
    #     RESET = '\033[0m'

    #     # Print calibration and NVS status
    #     print("Calibration and NVS Status:")
    #     for variable_name, data in self.calibration_test_results.items():
    #         calibration_result = GREEN + "Success" + RESET if data["calibrated"] else RED + "Failure" + RESET
    #         nvs_result = GREEN + "Success" + RESET if data["nvs"] else RED + "Failure" + RESET
    #         print(f"{variable_name}: Calibration - {calibration_result}, NVS - {nvs_result}")

    #     #-----------------------SAVES THE DATASET-----------------------------------------------------------------

    #     print("Saving dataset on CSV")

    #     # Open CSV file in write mode
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file)

    #         # Write header
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Calibration", "Dataset"])

    #         # Write variable headers
    #         writer.writerow(
    #             ["V1_DMM", "V1_TWIST", "V2_DMM", "V2_TWIST", "VH_DMM", "VH_TWIST", "I1_DMM", "I1_TWIST", "I2_DMM", "I2_TWIST", "IH_DMM", "IH_TWIST"]
    #         )

    #         # Write data
    #         max_length = max(len(self.calibration_test_results[key]['DMM']) for key in self.calibration_test_results)
    #         for i in range(max_length):
    #             row = []
    #             for key in self.calibration_test_results:
    #                 if len(self.calibration_test_results[key]['DMM']) > i:
    #                     row.append(f"{self.calibration_test_results[key]['DMM'][i]:.3f}")
    #                 else:
    #                     row.append("")
    #                 if len(self.calibration_test_results[key]['TWIST']) > i:
    #                     row.append(f"{self.calibration_test_results[key]['TWIST'][i]:.3f}")
    #                 else:
    #                     row.append("")
    #             writer.writerow(row)

    #         # Write footer
    #         writer.writerow(["------", "------"])

    #     #-----------------------SAVES THE CALIBRATION PARAMETERS-----------------------------------------------------------------
    #     # Write the parameters to a CSV file
    #     print("Saving coefficients parameters on CSV")
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter=',')  # Use comma as delimiter for CSV
    #         # Write the separator and test header
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Calibration", "Parameters"])
    #         # Write the header
    #         writer.writerow(["Variable", "gain", "offset", "r2"])

    #         # Iterate over each variable in calibration_test_results
    #         for variable_name, data in self.calibration_test_results.items():
    #             # Extract calibration parameters
    #             gain = data["gain"]
    #             offset = data["offset"]
    #             r2 = data["r2"]
    #             # Write the parameters to the CSV file
    #             writer.writerow([variable_name, gain, offset, r2])

    #         # Write the closing separator
    #         writer.writerow(["------", "------"])

    #     # Update test result based on the outcome of the test
        
    #     self.calibration_test_result = True


    # def efficiency_test(self):
    #     """
    #     Perform the efficiency test.

    #     This method performs the efficiency test by varying the voltage and current levels according to predefined
    #     steps and ranges. It iterates through different voltage and current values, adjusts the duty cycle, and collects
    #     measurements for each step. The collected data is then saved to a CSV file.

    #     Parameters:
    #     None

    #     Returns:
    #     None
    #     """

    #     VHigh_low = 12
    #     VHigh_high = 84
    #     VHigh_steps = 6
    #     VHigh_values_up = np.linspace(VHigh_low,VHigh_high,VHigh_steps)
    #     VHigh_values_down = np.linspace(VHigh_low,VHigh_high,VHigh_steps)
    #     ILow_low = 1
    #     ILow_high = 6
    #     ILow_steps = 6
    #     ILow_values_up = np.linspace(ILow_low,ILow_high,ILow_steps)
    #     ILow_values_down = np.linspace(ILow_low,ILow_high,ILow_steps)
    #     Duty_low = 0.1
    #     Duty_high = 0.8
    #     Duty_steps = 6
    #     Duty_values_up = np.linspace( Duty_low,Duty_high,Duty_steps)
    #     Duty_values_down = np.linspace( Duty_low,Duty_high,Duty_steps)

    #     #Power supply setup
    #     Vhigh_value = 20
    #     Ihigh_limit = 6.0

    #     #Power supply setup
    #     self.PowerSupply.setVoltage_V(Vhigh_value)         #sets the power supply voltage to 20V
    #     self.PowerSupply.setCurrentLimit_A(Ihigh_limit)    #sets the power supply current limit to 6A
    #     self.PowerSupply.powerON()                         #turns the power supply ON

    #     self.ActiveLoad.set_mode('CC')                      #sets the active load to current mode
    #     self.ActiveLoad.set_current_A(0)                      #sets the active laod current to zero
    #     self.ActiveLoad.enable_input(True)                  #turns the active load ON

    #     for test_voltage in VHigh_values_up:
    #         self.PowerSupply.setVoltage_V(test_voltage)
    #         time.sleep(self.short_delay)

    #         for test_current in ILow_values_up:
    #             self.ActiveLoad.set_current_A(test_current)

    #             for leg_to_test in self.leg_list:

    #                 SPIN_Comm.SendCommand(self.DUT,"DRIVER",leg_to_test,"ON")
    #                 SPIN_Comm.SendCommand(self.DUT,"CAPA",leg_to_test,"ON")
    #                 SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"ON")
    #                 SPIN_Comm.SendCommand(self.DUT,"DUTY",leg_to_test, Duty_low) #sets the duty low just in case
    #                 SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

    #                 for duty in Duty_values_up:
    #                     SPIN_Comm.SendCommand(self.DUT,"DUTY",leg_to_test, duty)

    #                     self.efficiency_duty[leg_to_test]["duty"].append(duty)
    #                     for key in self.efficiency_TWIST_results[leg_to_test].keys():
    #                         self.efficiency_TWIST_results[leg_to_test][key].append(self.get_TWIST_avg_measurement(key))

    #                     for key in self.efficiency_DMM_results[leg_to_test].keys():
    #                         self.efficiency_DMM_results[leg_to_test][key].append(self.get_DMM_measurement(key))

    #                 for duty in Duty_values_down: SPIN_Comm.SendCommand(self.DUT,"DUTY",leg_to_test, duty)

    #                 SPIN_Comm.SendCommand(self.DUT,"DRIVER",leg_to_test,"OFF")
    #                 SPIN_Comm.SendCommand(self.DUT,"CAPA",leg_to_test,"OFF")
    #                 SPIN_Comm.SendCommand(self.DUT,"LEG",leg_to_test,"OFF")
    #                 SPIN_Comm.SendCommand(self.DUT,"IDLE")
    #         print(ILow_values_down)
    #         for test_current in ILow_values_down: self.ActiveLoad.set_current_A(test_current)
    #     for sweep_down_voltage in VHigh_values_down: self.PowerSupply.setVoltage_V(sweep_down_voltage)


    #     #-----------------------SAVES THE DATASET-----------------------------------------------------------------

    #     # Write the dataset to a CSV file
    #     with open(self.CSVPathTest, 'a', newline='', encoding='UTF8') as file:
    #         writer = csv.writer(file, delimiter=',')  # Use comma as delimiter for CSV
    #         # Write the separator and test header
    #         writer.writerow(["------", "------"])
    #         writer.writerow(["Efficiency", "Dataset"])

    #         # Write header for LEG1
    #         writer.writerow(["Data", "LEG1"])
    #         writer.writerow(["duty", "V1T", "I1T", "VHT", "IHT", "VLD", "ILD", "VHD", "IHD"])

    #         # Write individual test results for LEG1
    #         for idx, duty in enumerate(Duty_values_up):
    #             writer.writerow([duty] + [self.efficiency_TWIST_results["LEG1"][key][idx] for key in self.efficiency_TWIST_results["LEG1"]] +
    #                             [self.efficiency_DMM_results["LEG1"][key][idx] for key in self.efficiency_DMM_results["LEG1"]])

    #         # Write header for LEG2
    #         writer.writerow(["Data", "LEG2"])
    #         writer.writerow(["duty", "V2T", "I2T", "VHT", "IHT", "VLD", "ILD", "VHD", "IHD"])

    #         # Write individual test results for LEG2
    #         for idx, duty in enumerate(Duty_values_up):
    #             writer.writerow([duty] + [self.efficiency_TWIST_results["LEG2"][key][idx] for key in self.efficiency_TWIST_results["LEG2"]] +
    #                             [self.efficiency_DMM_results["LEG2"][key][idx] for key in self.efficiency_DMM_results["LEG2"]])

    #         # Write the closing separator
    #         writer.writerow(["------", "------"])

    #     self.efficiency_test_result = True

    # def buck_test(self):
    #     # Perform buck test
    #     # Update test result based on the outcome of the test
    #     self.buck_test_result = True

    # def boost_test(self):
    #     # Perform boost test
    #     # Update test result based on the outcome of the test
    #     self.boost_test_result = True



    # Configure the serial port parameters


    # def send_command(self, command):
            
    #     self.DUT.write(command.encode('utf-8'))
    #     # Wait for a response (optional)
    #     time.sleep(1)
    #     # Read the response (optional)
    #     response = self.DUT.read_all().decode()
    #     print(f"Response: {response}")

    def rx(self, text):
        self.buffer = text
        if '\n' in self.buffer:
            datas_left = ''
            cr_idx = self.buffer.rfind('\n')
            txt_to_read = self.buffer[:cr_idx]
            if 'begin record' in txt_to_read:
                idx = self.buffer.find('begin record\r\n') # FIXME: no so clear with \n
                txt_to_read = txt_to_read[idx+14:]
                self.state = RECORD
                print("here begin")
            if 'end record' in txt_to_read:
                print("here end")
                idx = txt_to_read.find('end record')
                txt_to_read = txt_to_read[:idx]
                lines = txt_to_read.split("\r\n")
                datas_left = self.save_datas(lines)
                self.state  = IDLE

               

            with open(f"{datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}-record.txt", "w+") as ff:
                ff.write(self.f.getvalue())
            self.f.close()
            self.f = io.StringIO()

            if self.state == RECORD:
                lines = txt_to_read.split("\r\n")
                datas_left = self.save_datas(lines)

            self.buffer = datas_left + self.buffer[cr_idx:] 
        return text

    def tx(self, text):
        return text

    def __del__(self):
        if self.f:
            if not self.f.closed:
                self.f.close()


    def save_datas(self, lines):
        datas_left = ''
        output_text = ''
        # the last data should be '' 
        if lines[-1] != '':
            datas_left = lines.pop()
        else:
            lines.pop()
        for k, line in enumerate(lines):
            if "#" in line:
                output_text += "{}\n".format(line[1:])
            else:
                try:
                    nombre = struct.unpack('>f', bytes.fromhex(line))[0]
                    output_text += "{}\n".format(nombre)
                except:
                    nombre = 0.0
                    output_text += "{}\n".format(line)
        return output_text

    def to_dataFrame(self, filename):
        """ filename is the name of file got datas dump from a ScopeMimicry object 
        the format is : two first line with # at beginning
        in the first line there's a list of column names
        in the second line a number: the index of the final data recorded
        each other lines are 8 bytes in hex format and represent float datas.
        """
        with open(filename, "r") as f:
            file = f.readlines()

        line1 = file.pop(0)
        file.pop(0)
        if "#" or " " in file[0] or file[0].startswith(" "):
            # print(file[0])
            line2 = file.pop(0)
            idx = int(line2.replace("#", "").strip())
        else:
            idx = None
        file.pop(0)
        clean_lines = [line.strip() for line in file if line.strip()]
        str_datas = []
        for k, str_dat in enumerate(clean_lines):
            if str_dat == 'end record' :
                str_datas = clean_lines[:k]
        # print('clean_lines : ', str_datas)
        if str_dat == [] :
            datas = np.array([float(line) for line in str_datas], dtype=float)
        else :
            datas = np.array([float(line) for line in clean_lines], dtype=float)
        
        names = line1.replace("#", "").split(",")
        num_columns = len(names) - 1
        while len(datas) % num_columns != 0:
            datas = datas[:-1]
        if len(datas) % num_columns != 0:
            raise ValueError(f"Le nombre total d'éléments ({len(datas)}) n'est pas divisible par le nombre de colonnes ({num_columns}).")
        # datas = np.fromiter(file, dtype=float)
        
        datas = datas.reshape(-1, len(names) - 1)
        if idx:
            datas = np.roll(datas, -(idx + 1), axis=0)
        df = pd.DataFrame(datas, columns=names[:-1])
        return df

    
    def plot_df(self, df):
        """ plot dataframe in 3 different axes 
        first axes are voltages
        second axe are currents
        third are others...
        by convention we begin column name with 'V' when it is voltage
        we begin column name with 'I' when it is current
        """
        fig, axs = plt.subplots(3, 1)
        tics = np.arange(len(df))
        try:
            df["V_Low_estim"] = df["duty_cycle"] * df["V_high"]
        except:
            pass
        if "k_acquire" in df:
            del df["k_acquire"]
        for s in df:
            if s.startswith("V"):
                axs[0].step(tics, df[s], label=s)
            elif s.startswith("I"):
                axs[1].step(tics, df[s], label=s)
            else:
                axs[2].step(tics, df[s], label=s)
        for ax in axs:
            ax.legend()
            ax.grid()
        return fig





    def simuTest2(self):
        #Power supply setup
        self.PowerSupply.setVoltage_V(25)                          #sets the power supply voltage to 20V
        self.PowerSupply.setCurrentLimit_A(1)                    #sets the power supply current limit to 1A
        self.PowerSupply.powerON()                                #turns the power supply ON
        self.ActiveLoad.enable_input(False)

        #DUT setup
        SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG1",0.5)
        SPIN_Comm.SendCommand(self.DUT,"DUTY","LEG2",0.5)
        SPIN_Comm.SendCommand(self.DUT, "REFERENCE", "LEG1", "V1", 25/2)
        SPIN_Comm.SendCommand(self.DUT,"DRIVER","LEG1","ON")
        SPIN_Comm.SendCommand(self.DUT,"CAPA","LEG1","ON")
        SPIN_Comm.SendCommand(self.DUT,"LEG","LEG1","ON")
        SPIN_Comm.SendCommand(self.DUT,"POWER_ON")

        print('VH : ', self.get_DMM_measurement('VH'), '\n')
        print('VL : ', self.get_DMM_measurement('VL'), '\n')
        print('IH : ', self.get_DMM_measurement('IH'), '\n')
        print('IL : ', self.get_DMM_measurement('IL'), '\n')

    def send_command(self, command):
        """
        Envoie une commande au microcontrôleur via le port série.
        """
        self.DUT.write((command + '\n').encode('utf-8'))

    def read_segment(self):
        """
        Lit un segment de données envoyé par le microcontrôleur.
        """
        segment_data = ""
        while True:
            if self.DUT.in_waiting > 0:
                # Lire ligne par ligne les données envoyées
                line = self.DUT.readline().decode('utf-8').strip()

                if line == "end record":
                    print("Fin de l'enregistrement des données.")
                    break
                else:
                    # Ajouter chaque ligne de données au segment
                    segment_data += line + '\n'

            else:
                # Petite pause pour ne pas surcharger le CPU
                time.sleep(0.01)

        return segment_data

    def read_scope_data(self):
        """
        Lit l'intégralité des données envoyées par le microcontrôleur,
        segment par segment, en envoyant des commandes de demande.
        """
        full_output = ""
        reading_data = False
        channels = []
        final_idx = None

        while True:
            if self.DUT.in_waiting > 0:
                line = self.DUT.readline().decode('utf-8').strip()

                # Détecter le début de l'enregistrement
                if "begin record" in line:
                    print("Début de l'enregistrement des données.")
                    reading_data = True
                    continue

                # Détecter la fin de l'enregistrement
                elif "end record" in line:
                    print("Fin de l'enregistrement.")
                    break

                # Si on lit des données et que ce n'est pas encore la fin
                if reading_data:
                    # Si la ligne commence par #, c'est une ligne d'information
                    if line.startswith("#"):
                        if not channels:
                            channels = line[1:].split(',')
                            print(f"Noms des canaux: {channels}")
                        elif final_idx is None:
                            final_idx = int(line.split()[-1])
                            print(f"Index final: {final_idx}")
                    else:
                        # Envoyer la commande pour demander un segment de données
                        self.send_command("NEXT_SEGMENT")
                        segment_data = self.read_segment()
                        full_output += segment_data

        return full_output, channels, final_idx



    def simuTest(self): 
        Vhigh_tension = 25
        self.PowerSupply.setCurrentLimit_A(3.0)
        self.PowerSupply.setVoltage_V(Vhigh_tension) #Fix to 25V
        self.PowerSupply.powerON()

        """
        # For unknow reason the active load only enable after a few mS after the test starts even with delay
        # It's probably waiting for the output voltage of the DC converter to stabilize before consuming current
        # it's better to use a real load
        """
        # self.ActiveLoad.set_mode('CR')                              # Constant resistance mode"
        # self.ActiveLoad.set_resistance_Ohm(10)
        # self.ActiveLoad.enable_input(True)                          # enable the active load

        # time.sleep(5) # Pause 2s to start active load


        if not self.recorded_data_DUT.serial_port.is_open:
            self.recorded_data_DUT.serial_port.open()
        
        # Change the code test_sensibilite to get value from specified in scope mimicry
        leg = 'LEG1'
        Vref = Vhigh_tension / 2
        

        Ts = 1e-4
        SPIN_Comm.SendCommand(self.recorded_data_DUT.serial_port,"POWER_ON")
        SPIN_Comm.SendCommand(self.DUT, "CAPA", leg, "ON")
        SPIN_Comm.SendCommand(self.DUT, "DRIVER", leg,"ON", delay=3)
        SPIN_Comm.SendCommand(self.recorded_data_DUT.serial_port, "TEST_SENSI", leg, Vref, delay=1) # does sensitivity test

        try:
            file_path = self.recorded_data_DUT.read_serial()
        except KeyboardInterrupt:
            print("Process interrupted by user.")
        finally:
            self.recorded_data_DUT.close()
        # self.PowerSupply.powerOFF()


        filename_txt = file_path + '.txt'
        filename_csv = file_path + '.csv'
        
      
        # Dictionnaire pour stocker les colonnes
        columns = {}

        # Lecture du fichier CSV
        with open(filename_csv, mode='r', encoding='utf-8-sig') as file:
            reader = csv.DictReader(file)

            # Initialisation des listes pour chaque colonne
            for column_name in reader.fieldnames:
                print(column_name)
                columns[column_name] = []
            
            # Parcours de chaque ligne du fichier CSV
            for row in reader:
                for column_name in reader.fieldnames:
                    columns[column_name].append(float(row[column_name]))
                

        beginning_step = 600 # step begins at the 600th point
        ending_step = 800 # step ends at the 800th point
        if leg == 'LEG1' : 
            Vlow_name = 'V1_low'
            Ilow_name = 'I1_low'
            Power_name = 'P1_output'
            Temp_name = 'T1'
        else : 
            Vlow_name = 'V2_low'
            Ilow_name = 'I2_low'
            Power_name = 'P1_output'
            Temp_name = 'T2'
        Record_Size = len(columns[Ilow_name]) # 1000
        # print(columns[Vlow_name][beginning_step:ending_step])
        # print(columns[Vlow_name])
        voltagePortion = columns[Vlow_name][beginning_step:ending_step]
        currentPortion = columns[Ilow_name][beginning_step:ending_step]
        sim_time = Record_Size * Ts
        final_value_voltage = voltagePortion[-1]
        settling_threshold = 0.05
        time_values = np.linspace(0, sim_time, Record_Size)
        timePortion = time_values[beginning_step:ending_step]

        # establishing output powers
        # columns[Power_name] = []
        # for i in range(Record_Size):
        #     columns[Power_name].append(columns[Vlow_name][i] * columns[Ilow_name][i])

        columns[Power_name] = []
        columns["P_in"] = []
        columns["efficiency"] = []

        for i in range(Record_Size):
            V_low = columns[Vlow_name][i]
            I_low = columns[Ilow_name][i]
            V_high = columns["V_high"][i]
            I_high = columns["I_high"][i]

            P_out = V_low * I_low
            P_in = V_high * I_high
            efficiency = P_out / P_in if P_in != 0 else 0

            columns[Power_name].append(P_out)
            columns["P_in"].append(P_in)
            columns["efficiency"].append(efficiency)
                
        # columns["P2_low"] = []
        # for i in range(Record_Size):
        #     columns["P2_low"] = columns["V2_low"][i] * columns["I2_low"][i]

        # columns["T1"] = []
        # columns[Temp_name] = []

        # print(columns.keys())

        results = {}

        # # Maintenant, chaque colonne est stockée dans une variable distincte
        # # par exemple, si le fichier contient une colonne "Nom", vous pouvez y accéder via `columns['Nom']`

        # ### calculation of settling time 


        # Initialisation des variables
        stable = False
        settling_time = np.nan

        # Recherche du temps de stabilisation
        for j in range(len(voltagePortion)):
            if abs(voltagePortion[j] - final_value_voltage) < abs(settling_threshold * final_value_voltage):
                stable = True
                for k in range(j, len(voltagePortion)):
                    if abs(voltagePortion[k] - final_value_voltage) > settling_threshold * final_value_voltage:
                        
                        stable = False
                        break
                if stable:
                    settling_time = timePortion[j]
                    break

        # Convertir le temps de stabilisation en millisecondes
        settling_time_ms = settling_time  * 1000
        overshoot = ((max(voltagePortion) - final_value_voltage) / final_value_voltage) *  100
        print(max(voltagePortion))
        print(final_value_voltage)
        results["settling_time_ms"] = settling_time_ms
        results["overshoot"] = overshoot

        # Calcul des moyennes entre les points 200 et 550
        avg_start = 500
        avg_end = 600

        P_out_list = columns[Power_name][avg_start:avg_end]
        eff_list = columns["efficiency"][avg_start:avg_end]

        avg_P_out = sum(P_out_list) / len(P_out_list)
        avg_eff = sum(eff_list) / len(eff_list)

        # Calcul de l'amplitude crête à crête du courant I_low entre les points 900 et la fin
        I_low_list = columns[Ilow_name][900:]
        if I_low_list:
            peak_to_peak_I_low = max(I_low_list) - min(I_low_list)
        else:
            peak_to_peak_I_low = float('nan')  # en cas de liste vide

        results["I_low_peak_to_peak (900-end)"] = peak_to_peak_I_low

        results["avg_P_out (500-600)"] = avg_P_out
        results["avg_efficiency (500-600)"] = avg_eff

        # Affichage du résultat
        print(f"Settling Time (ms): {settling_time_ms}")
        print(f"Overshoot (%): {overshoot}")

        # self.CSVPathTest = f"twist_log_20250520-155816.csv" # just for the test 

        with open(self.CSVPathTest, mode='a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([])  # ligne vide pour séparation
            writer.writerow(["--- SIMU TEST RESULTS ---"])
            for key, value in results.items():
                writer.writerow([key, value])

        with open(self.CSVPathTest, mode='a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([])  # ligne vide pour séparer
            writer.writerow(["--- SIMU SIGNALS ---"])
            
            # Écriture des entêtes
            header = ['Time (s)']
            header += list(columns.keys())
            writer.writerow(header)

            # Transposer les colonnes pour les écrire ligne par ligne
            num_rows = len(next(iter(columns.values())))
            for i in range(num_rows):
                row = [time_values[i]]  # Time
                row += [columns[col][i] for col in columns]
                writer.writerow(row)






    def cycling_test(self):
        """
        Perform the cycling test.

        This method performs a thermal cycling test of a leg of the TWIST board.
        It raises the current to achieve a certain temperature. 
        Keeps this temperature via a hysteresis. 
        Cools down the converter. 

        Parameters:
        None

        Returns:
        None
        """
        print("----------------------")
        print("Start Cycling Test")
        #Power supply setup
        Vhigh_value = 50
        Ihigh_limit = 5.0
        
        temp_limit = 35.0

        voltage_ref = 25
        current_ref = 1.0

        #Power supply setup
        self.PowerSupply.setVoltage_V(Vhigh_value)                          #sets the power supply voltage to 20V
        self.PowerSupply.setCurrentLimit_A(Ihigh_limit)                    #sets the power supply current limit to 1A
        self.ActiveLoad.enable_input(False)
        SPIN_Comm.SendCommand(self.DUT,"IDLE", delay=1)


        print("Starting cycle")
        self.PowerSupply.powerON()                                 #turns the power supply ON
       
        """
        The active load is disabled it's better to use a real load for getting results
        from scope mimicry
        I believe the active load is waiting for a stable voltage before starting to act like a real load
        """
        # self.ActiveLoad.set_mode('CR')                              # Constant current mode"
        # self.ActiveLoad.set_resistance_Ohm(4)
        # self.ActiveLoad.enable_input(True)                          # enable the active load

        leg_to_test = 'LEG1'

        if leg_to_test == 'LEG1' : 
            Vlow_name = 'V1'
            Ilow_name = 'I1'
        else : 
            Vlow_name = 'V2'
            Ilow_name = 'I2'

        # -------------- Test for the new temperature cycling ------  
        

        # Optional: Add timestamp to filename to avoid overwriting
        timestamp_str = datetime.now().strftime("%Y%m%d-%H%M%S")
        filename = f"twist_log_{timestamp_str}.csv"
        self.CSVPathTest = f"twist_log_{timestamp_str}.csv"
 

    # test.read_qr()
        print("Testing", leg_to_test)
        message = SPIN_Comm.SendCommand(self.DUT, "IDLE")
        print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "POWER_OFF")
        print(message)

        message = SPIN_Comm.SendCommand(self.DUT, "CAPA", leg_to_test, "ON", delay=1)
        message = SPIN_Comm.SendCommand(self.DUT, "BUCK", leg_to_test, "ON", delay=1); print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "BOOST", leg_to_test, "OFF", delay=1); print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "DRIVER",leg_to_test,"ON", delay=1)
        print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "REFERENCE",leg_to_test, Vlow_name, voltage_ref, delay=1)
        print(message)
        

        message = SPIN_Comm.SendCommand(self.DUT, "LEG",leg_to_test,"ON", delay=1)
        print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "POWER_ON", delay=1)
        print(message)
    


        print(f"\nTemp limit {temp_limit}")
    
        # Open CSV file in write mode
        with open(filename, mode='w', newline='') as file:
            writer = csv.writer(file)

            # Write header
            writer.writerow(["Timestamp", "T2 (°C)", "I (A)", "I(DMM) (A)"])

            # Initial measurements

            T2_meas = self.get_TWIST_avg_measurement('T2')
            I2_meas = self.get_TWIST_avg_measurement(Ilow_name)

            init_temp = T2_meas
            start_time = datetime.now()

            # Main loop
            while T2_meas < temp_limit:
                T2_meas = self.get_TWIST_avg_measurement('T2')
                I_meas = self.get_TWIST_avg_measurement(Ilow_name)
                V_meas = self.get_TWIST_avg_measurement(Vlow_name)
                I_meas_DMM = self.get_DMM_measurement('IL')
                timestamp = datetime.now().isoformat()

                writer.writerow([timestamp, round(T2_meas, 3), round(I_meas, 3), round(I_meas_DMM, 3)])
                print(f"{timestamp} | T2 = {T2_meas:.2f}°C | V = {V_meas:.3f} V | I = {I_meas:.3f} A | I(DMM) = {I_meas_DMM:.3f} A")

                time.sleep(1)

        print(f"\nData successfully saved to {filename}")

        # Final calculations
        end_time = datetime.now()
        end_temp = T2_meas
        delta_time = (end_time - start_time).total_seconds()
        delta_temp = end_temp - init_temp
        slope = delta_temp / delta_time if delta_time > 0 else float('inf')

        # Print summary
        print(f"\ninitial time = {start_time.isoformat()} | end time = {end_time.isoformat()} | delta time = {delta_time:.2f} s")
        print(f"initial temp = {init_temp:.2f}°C | end temp = {end_temp:.2f}°C | delta temp = {delta_temp:.2f}°C")
        print(f"temp / time  = {slope:.4f} °C/s")

        # -------------------------------
        # PLOT & EXPORT CSV
        # -------------------------------
        timestamps = []
        temps = []
        currents = []
        currents_DMM = []

        with open(filename, mode='r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                timestamps.append(datetime.fromisoformat(row["Timestamp"]))
                temps.append(float(row["T2 (°C)"]))
                currents.append(float(row["I (A)"]))
                currents_DMM.append(float(row["I(DMM) (A)"]))

        # Convert timestamps to seconds relative to the first one
        time_zero = timestamps[0]
        elapsed_seconds = [(t - time_zero).total_seconds() for t in timestamps]

        # Plot
        plt.figure(figsize=(10, 6))
        plt.plot(elapsed_seconds, temps, label="T2 (°C)", linewidth=2)
        plt.plot(elapsed_seconds, currents, label="I (A)", linewidth=2)
        plt.plot(elapsed_seconds, currents_DMM, label="I(DMM) (A)", linewidth=2)

        plt.xlabel("Time (s)", fontsize=12)
        plt.ylabel("Measured Value", fontsize=12)
        plt.title("TWIST Thermal Test", fontsize=14)
        plt.legend(loc='upper left')
        plt.grid(True)
        plt.tight_layout()

        # Save as SVG
        plot_filename = filename.replace(".csv", ".svg")
        plt.savefig(plot_filename)
        print(f"Plot saved to {plot_filename}") 

        # message = SPIN_Comm.SendCommand(self.DUT, "LEG","LEG1","OFF")
        # print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "LEG", leg_to_test, "OFF", delay=1)
        print(message)
        message = SPIN_Comm.SendCommand(self.DUT, "POWER_OFF", delay=1)
        print(message)
        message1 = SPIN_Comm.SendCommand(self.DUT, "IDLE", delay=1)
        print(message1)
        plt.show()

        
        # self.ActiveLoad.set_current_A(0)                            #  Set current (to 0 to turn off)
        # self.ActiveLoad.enable_input(False)                          # enable the active load

