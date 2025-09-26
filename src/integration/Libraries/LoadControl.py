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


import Libraries.bk8500 as bk8500
import time

class IT8512BLoadController:
    def __init__(self, serial_obj, serial_add, baudrate, parity, timeout):
        '''
        IT8512BLoadController Constructor:

        Initializes the IT8512B Load Controller object.

        Parameters:
            - serial_obj: Serial object for communication
            - serial_add: Address of the device
            - baudrate: Baudrate for serial communication
            - parity: Parity for serial communication
            - timeout: Timeout for serial communication
        '''
        self.serial_obj = serial_obj
        self.serial_add = serial_add
        self.baudrate = baudrate
        self.parity = parity
        self.timeout = timeout
        self.serial_obj.baudrate = baudrate
        self.serial_obj.parity = parity
        self.serial_obj.timeout = timeout
        self.serial_obj.open
        self.verbose = False

    def set_verbose_mode(self, verbose):
        '''
        Set the verbose mode on/off:

        returns details of the operations done by the active load.

        Parameters:
            - verbose: True for verbose and False for quiet mode
        '''
        self.verbose = verbose
        time.sleep(0.2)


    def set_current_A(self, current_A):
        '''
        Set Current Function:

        Sets the current when the load is in Constant Current (CC) mode.

        Parameters:
            - current_A: Current value in amps
        '''
        bk8500.set_current(self.serial_obj, self.serial_add, current_A, self.verbose)
        time.sleep(0.2)

    def set_voltage_V(self, voltage_V):
        '''
        Set Voltage Function:

        Sets the voltage in Constant Voltage (CV) mode.

        Parameters:
            - voltage_V: Voltage value in volts
        '''
        bk8500.set_voltage(self.serial_obj, self.serial_add, voltage_V, self.verbose)
        time.sleep(0.2)

    def set_resistance_Ohm(self, resistance_Ohm):
        '''
        Set Resistance Function:

        Sets the resistance in Constant Resistance (CR) mode.

        Parameters:
            - resistance_Ohm: Resistance value in ohms
        '''
        bk8500.set_resistance(self.serial_obj, self.serial_add, resistance_Ohm, self.verbose)
        time.sleep(0.2)

    def set_mode(self, mode):
        '''
        Set Mode Function:

        Sets the mode of the device.

        Modes:
            - "CC": Constant Current mode
            - "CV": Constant Voltage mode
            - "CW": Constant Power mode
            - "CR": Constant Resistance mode

        Parameters:
            - mode: Desired mode ("CC", "CV", "CW", or "CR")
        '''
        bk8500.set_mode(self.serial_obj, self.serial_add, mode, self.verbose)
        time.sleep(0.2)

    def set_remote(self, remote_mode):
        '''
        Set Remote Function:

        Sets the remote mode of the device.

        Parameters:
            - remote_mode: True for remote mode, False for local mode
        '''
        bk8500.set_remote(self.serial_obj, self.serial_add, remote_mode, self.verbose)
        time.sleep(0.2)

    def enable_input(self, enable_mode):
        '''
        Enable Input Function:

        Enables or disables the input of the device.

        Parameters:
            - enable_mode: True to enable input, False to disable input
        '''
        bk8500.enable_input(self.serial_obj, self.serial_add, enable_mode, self.verbose)
        time.sleep(0.2)

    def set_safety_settings(self, max_volt_V, max_curr_A, max_power_W):
        '''
        Set Safety Settings Function:

        Sets the safety settings of the device.

        Parameters:
            - max_volt_V: Maximum voltage in volts
            - max_curr_A: Maximum current in amps
            - max_power_W: Maximum power in watts
        '''
        bk8500.set_max_voltage(self.serial_obj, self.serial_add, max_volt_V, self.verbose)
        bk8500.set_max_current(self.serial_obj, self.serial_add, max_curr_A, self.verbose)
        bk8500.set_max_power(self.serial_obj, self.serial_add, max_power_W, self.verbose)
        time.sleep(0.2)
