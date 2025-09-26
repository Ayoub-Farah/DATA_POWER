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

# Python modules import
from Libraries.factory_test import FactoryTest
import time
from Libraries import test_finished
import Libraries.prog_utils
start_time = time.time()

SN_tested_12_03_2024 = [ "TWTB0M6K1NV4U",       #00 - DONE
                "TWTZMVPD0D9UL",      #01 - DONE
                "TWT4REU4BAW7N799",   #02 - DONE  
                "TWTB7WH0JZZ7Z",      #03 - TO BE REDONE
                "TWTE8Y5YE3WV0",      #04 - DONE
                "TWTSTAVFRQXBX",      #05 - DONE
                "TWTT5765PN1VN",      #06 - DONE
                "TWT4S3EGMDCZH",      #07 - DONE
                "TWT8BKLQHED8W",      #08 - DONE 
                "TWT9WCBDRXPKK",      #09 - DONE
                "TWT4VA1DM57FP",      #10 - DONE           
                "TWT1Y6J7D6FSY"]      #11 - DONE


SN_tested_14_03_2024 = [ "TWTZXDM2SVT47",       #00 - DONE
                "TWTGMAHK5ATC7"]      #01 - DONE

SN_tested_15_03_2024 = ["TWTN2B2G1ZWZU", #DONE
"TWT0SFWQ6VGCP", #DONE
"TWT8L6HVV1J2B", #DONE
"TWTE8JZNRSHCR", #DONE
"TWT37SHMUZLW8", #DONE - TO ERASE TWICE
"TWTHRRP2K3UHY", #DONE - FIRST TEST NO CURRENT
"TWTNWQL5ETZ46", #DONE
"TWTX4HUDEMUJE", #DONE - TWO PASSES (CONNEXION ERROR)
"TWTRHQ95PAG18", #DONE
"TWTAEBE6P8JL3"]



SN_to_test = [ "Basic" ]      

test_to_do = 0

test = FactoryTest(TempMeas=False)
red_failure = test.RED + "Failure" + test.RESET
error_failure = test.RED + "-------ERROR-------" + test.RESET
abort_failure = test.RED + "------Aborting------" + test.RESET


try:
    # print('here')
    test.cycling_test()
    test.simuTest()
    # test.read_qr()
    # test.get_test_report()
    # test.get_test_number()
    
    # print("ici")
    # test.program_DUT()
    # if test.get_test_results_DUT("program") is False: raise ValueError("Program")  

    # test.feeder_test()          #runs the feeder test. Limits the current to 300mA and validates the feeder operation.
    # if test.get_test_results_DUT("feeder") is False: raise ValueError("Feeder")  

    # test.communication_test()   #runs the communication tests. Validates sync, RS485 and analog. CAN Bus still not working.
    # if test.get_test_results_DUT("Sync") is False: raise ValueError("Sync")  

    # test.driver_switch_test()   #runs the driver switch test. validates power on and off by measuring low side voltage at duty = 0.5.
    # if test.get_test_results_DUT_LEG("LEG1", "driver") is False : raise ValueError("Driver1")  
    # if test.get_test_results_DUT_LEG("LEG2", "driver") is False : raise ValueError("Driver2")  

    # test.open_circuit_test()    #runs the open circuit test. Sweeps the duty cycle from 0.1 to 0.9 and validates its operation. 
    # if test.get_test_results_DUT_LEG("LEG1", "open circuit") is False : raise ValueError("OC1")  
    # if test.get_test_results_DUT_LEG("LEG2", "open circuit") is False : raise ValueError("OC2")  

    # test.capacitor_test()       #runs the capacitor test. Measures the peak of a step response with capacitor on and off. 
    # if test.get_test_results_DUT_LEG("LEG1", "capa") is False : pass#raise ValueError("Capacitor1")  
    # if test.get_test_results_DUT_LEG("LEG2", "capa") is False : pass#raise ValueError("Capacitor2")  

    # test.calibration_test()     #runs the calibration test. Measures raw data from the board vs. DMM reference measurements. Parameters are auto-calculated and flashed to the NVS.

    # test.upload_test_report()

    # test.get_test_report()




    # test_finished.victory()

    # test.efficiency_test()

except ValueError as e:
    print(f"\n{error_failure}\n")
    if str(e) == "Program":
        print(f"Program Test {red_failure}. Check the DUT's SPIN connection or bootloader.")
    elif str(e) == "Feeder":
        print(f"Feeder Test {red_failure}. Check the feeder jumper. check for short/open circuits. ")
    elif str(e) == "Sync":
        print(f"Sync Test {red_failure}. Check B2 pin is soldered. ")
    elif str(e) == "Driver1":
        print(f"Driver Test LEG1 {red_failure}. Check LEG1 Driver or PC12/19 GPIO. ")
    elif str(e) == "Driver2":
        print(f"Driver Test LEG2 {red_failure}. Check LEG2 Driver or PC13/22 GPIO. ")
    elif str(e) == "OC1":
        print(f"Open Circuit Test LEG1 {red_failure}. Check LEG1 PWM signals. ")
    elif str(e) == "OC2":
        print(f"Open Circuit Test LEG2 {red_failure}. Check LEG2 PWM signals. ")
    elif str(e) == "Capacitor1":
        print(f"Capacitor Test LEG1 {red_failure}. Check LEG1 capacitor MOSFET or PC6/7 GPIO signal.")        
        if test.get_test_results_DUT_LEG("LEG2", "capa") is False : 
            print (f"Capacitor Test LEG2 {red_failure}. Check LEG2 capacitor MOSFET or PB7/56 GPIO signal.")
    elif str(e) == "Capacitor2":
        print(f"Capacitor Test LEG2 {red_failure}. Check LEG2 capacitor MOSFET or PB7/56 GPIO signal.")
    else:
        print("Unexpected error:", e)
    print(f"\n{abort_failure}\n")

finally:
    test.all_off()
    # Calculate elapsed time
    elapsed_time = time.time() - start_time
    print(f"Elapsed time: {elapsed_time:.2f} seconds")
