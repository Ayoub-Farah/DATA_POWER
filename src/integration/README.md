# Converter characterization
This repository contains the scripts used to characterize a DC DC converter. It is organized with one file per inteded characterization, i.e Coefficients.py is to get the calibration coefficients, Efficiency.py is to measure the efficiency of the converter, Accuracy.py to measure the accuracy of the sensors.
The readme is organized per inteded characterization. The first section is about obtaining the calibration coefficients. The second section is about the efficiency, the third one about the accuracy.

## Getting started
The scripts are developped to run under windows 10.
Other windows distributions have not been tested.

## Setup
The setup is identical for all the characterization.

### Setup : instruments
One power supply, four DMMs (Digital MultiMeter), one active load and one STLINKV3 debugger are used in the setup.

#### Setting the operating point
 - Power supply : 1 EA-PSI-9750-06
 - Active load : 1 ITECH IT8512B and a RS232 to USB interface B&K IT-E132
 - Debugger : 1 STLINKV3
note : on the actual version auto-detection of COM PORT is disabled. So for proper work, please set the correct COM Port of each instrument in the Settings.py file. Information can be found in "Ports" section of device manager of windows

#### Measuring input and output voltages and currents
 - Digital Multimeters : 4 KEITHLEY K2000 and a GPIB Interface KEYSIGHT 82357A
 - Current sensing : 2 15FR010E 5W, 10mOhm +/- 1% precision current sensing resistor

A computer with windows 10 is used to run the script and collect the data.

The test setup is the following :

![Schematic_setup](Doc/Architecture_setup.png "Schematic_setup")

### Setup : communication

Communication architecture is the following :

![Architecture communication](Doc/Architecture_communication.PNG "Architecture_communication")

### Instruments setup
The load and the DMM must have settings set via the hardware control panel.
They are the following : 

- Active load : 
Menu -> Config -> Address ser -> address = 0
Make sure the serial communication parameters are set the following :
baudRate = 9600
bytesize = 8
parity = "N"
stopbits = 2

- DMM :
All the DMM should be configured in SCPI communication mode. To do so, go to front panel and type the buttons : 

Shift -> GPIB -> GPIB -> ON 
Shift -> GPIB -> LANG -> SCPI

As each DMM has a specific function (read current low, high, voltage low and high), every DMM should have a specific address.

To change the address :

Shift -> GPIB -> Address

The following array describes the addresses according to the function :

|               Function               | Address |
|:------------------------------------:|:-------:|
| IHighResource (voltage across shunt) |    1    |
|             VHighResource            |    2    |
|             VLowResource             |    3    |
|  ILowResource (voltage across shunt) |    4    |

### Firmware
Before running the script, make sure the board under test (BUT) is flashed with the code in the core folder.

A procedure detailing how to flash the BUT is available :
https://gitlab.laas.fr/owntech/tutorials/-/wikis/Getting-Started-1-Blinky

### Python
To run, the script need python 3 (older versions have not been tested)
Make sure you install python from official website. Issues were encountered when python was installed from windows store.
Official website URL : https://www.python.org/
At python installation, check the box to add python to PATH.

### Requirements
With a terminal, go to the root of the repository and type the line
```pip install -r requirements.txt```

### Pyvisa
Pyvisa is a module listed in the requirements.txt file.
But it needs a VISA API installed on your computer. To do so, follow the tutorial: 
https://pyvisa.readthedocs.io/en/latest/introduction/getting.html

### Driver
To communicate, load and DMM requires extra hardware with specific drivers

- Driver for active load : B&K IT-E132 RS232/USB interface.
The windows 10 driver files are available in the IT-E132_Driver folder. 
You will find an installation procedure in Doc/IT-E132_Install_Driver document.

- Driver for power supply : it should automatically be installed when the power supply is connected.

## Known issues
The following issues are know and a way of handle them is given.
- B&K IT-E132 RS232/USB interface driver can randomly uninstall (especially when rebooting). Install it back (see procedure above).
- At the start of script, DMM can send back an error. Power it off then on and restart the script
- COM port of instrument can change, especially if deconnected and reconnected. Set back the correct COM PORT in settings.py

Now you are all set, you can start using the scripts ! We now describe how to run each script and what they produce.

## Coefficients
This script is intended to compute the calibration coefficients.

### How to run
With a terminal, go to the root of the repository and type the line
```python Coefficients.py```

You will the be guided by the script to type the test settings (test name, configuration,...). 
Just interact with the terminal !

### Flowchart
This python script handles multiple test instruments in order to compute calibration coefficients of the converter.
The flowchart of the script is the following :

![FlowChart_Coefficients](Doc/FlowChart_Coefficients.PNG "FlowChart_Coefficients")

### Output folder structure
The script generates its output in the folder 'Output_Coefficients' created at the root of the script folder.

### Outupt of the script
Each time the script is run, following folder/files are created into the Output_Coefficients folder : 
- one folder named "BoardSN"
- Inside this "BoardSN" folder, 4 files are created : VHigh.csv, IHigh.csv, VLow(x).csv, ILow(x).csv

![Output_Coefficients](Doc/OutputCoefficients.PNG "Output_Coefficients")

Each time the script is run, the file Coeffs.csv is also appended with a new line (under the last line). The first column of this new row contains the "BoardSN".

### Custom test
In the Settings.py file, variables can be changed to customize the test. 
The variable and its description are now listed

|              Variable              |                                 Description                                |
|:----------------------------------:|:--------------------------------------------------------------------------:|
|       CoeffNumberOfTWISTMeas       |         The number of twist averaged twist samples for each measure        |
| CoeffVHighCalibrationDutyPercent   | The duty cycle for the calibration of VHigh                                |
| CoeffVHighCalibrationLowCurrent_mA | The current drawn on low side for the calibration of VHigh                 |
| CoeffIHighCalibrationDuty_Percent  | The duty cycle for the calibration of IHigh                                |
| CoeffIHighCalibrationVHigh_mV      | The VHigh value for the calibration of IHigh                               |
| CoeffVLowCalibrationLowCurrent_mA  | The current drawn on low side for the calibration of VLow                  |
| CoeffVLowCalibrationVHigh_mV       | The VHigh value for the calibration of VLow                                |
| CoeffILowCalibrationDuty_Percent   | The duty cycle for the calibration of ILow                                 |
| CoeffILowCalibrationVHigh_mV       | The VHigh value for the calibration of ILow                                |
| CoeffStepMinVHigh_mV               | The minimal value of VHigh for the VHigh calibration                       |
| CoeffStepMaxVHigh_mV               | The maximal value of VHigh for the VHigh calibration                       |
| CoeffNumberOfVHighStep             | The number of VHigh steps for the VHigh calibration                        |
| CoeffStepMinILowCurrent_mA             | The minimal value of ILow for the ILow/IHigh calibration (value is shared) |
| CoeffStepMaxILowCurrent_mA             | The maximal value of ILow for the ILow/IHigh calibration (value is shared) |
| CoeffNumberOfILowCurrentStep           | The number of Ilow steps for the VHigh calibration                         |

## Efficiency
We now describe the process for the characterization of the efficiency of the converter:

### How to run
With a terminal, go to the root of the repository and type the line
```python Efficiency.py```

### Flowchart
This python script handles multiple test instruments in order to assess efficiency of power converters.
The flowchart of the script is the following : 

![Flowchart](Doc/Flowchart.PNG "Flowchart")

### Output folder structure
The script generates its output in the folder 'Output_Efficiency' created at the root of the script folder.

### Outupt of the script
The script generates the following files: 

|           File           |                                             Description                                             |
|:------------------------:|:---------------------------------------------------------------------------------------------------:|
|    testname_VHigh.csv    | csv file containing every variable monitored during the test                                        |
|  testname_VHigh_AVGD.csv | csv file containing variable monitored, averaged with the value NumberOfpointsPerDuty (settings.py) |
|    testname_histo.svg    | svg containing the distribution of every monitored variable                                         |
|    testname_Eff2D.png    | The efficiency on a 2-D projection, it concatenates every VHigh measures of the test                |
|    testname_Eff2D.svg    | Same as above but in svg                                                                            |
|   testname_3d_plot.html  | The efficiency on 3D, it concatenates every VHigh measures of the test                              |
| testname_3d_rel_err.html | The relative error on efficiency measure in 3D                                                      |
| testname_3d_rel_abs.html | The absolute error on efficiency measure in 3D                                                      |

A recap of the output folder is visible:

![Output_efficiency](Doc/OutputEfficiency.PNG "Output_efficiency")

### Custom test
In the Settings.py file, variables can be changed to customize the test. 
The variable and its  description are now listed

|           Variable          |                                        Description                                       |
|:---------------------------:|:----------------------------------------------------------------------------------------:|
| LoadInternalSafetyCurrent_A | The maximum current the load will accept to sink, in Ampere                              |
|  LoadInternalSafetyPower_W  | The maximum power the load will accept to sink, in Watt                                  |
|    NumberOfpointsPerDuty    | Number of samples read per duty, same for DMM and SPIN                                   |
|      duty_step_Percent      | The step of duty cycle when we send 'u' (or 'd') to SPIN                                 |
| NumberOfStepInDutySweep     | The number of times we increase D for ONE Duty sweep                                     |
| starting_duty_cycle_Percent | The value of duty cycle when we send 'r' to SPIN                                         |
| CurrentStepValue_i_mA       | The value of the current for the step i. A duty sweep is performed for each current step |
|                             |                                                                                          |

## Accuracy
We now describe the process for the characterization of the accuracy of the converter:

### How to run
With a terminal, go to the root of the repository and type the line
```python Accuracy.py```

You will the be guided by the script to type the test settings (test name, configuration,...). 
Just interact with the terminal !

### Flowchart
This python script handles multiple test instruments in order to assess accuracy of the sensors of the power converter.

The sensors measure VHigh, VLow (both legs), IHigh and ILow (both legs).

The flowchart of the script is the following : 

![Flowchart_Accuracy](Doc/FlowChart_Accuracy.png "Flowchart_Accuracy")

### Output folder structure
The script generates output in the folder 'Output_Accuracy' created at the root of the script folder.

### Outupt of the script
The script generates the following files: 

|               File               |                                   Description                                  |
|:--------------------------------:|:------------------------------------------------------------------------------:|
| TestName_AvgNum_All_Abs.png      | png file containing graph of all absolute errors (VHigh, VLow, IHigh and Ilow) |
| TestName_AvgNum_All_Abs.svg      | svg file containing graph of all absolute errors (VHigh, VLow, IHigh and Ilow) |
| TestName_AvgNum_All_Rel.png      | png file containing graph of all relative errors (VHigh, VLow, IHigh and Ilow) |
| TestName_AvgNum_All_Rel.svg      | svg file containing graph of all relative errors (VHigh, VLow, IHigh and Ilow) |
|  TestName_AvgNum_IHigh_AVGD.csv  | csv file containing the data for the error plot on IHigh measure               |
| TestName_AvgNum_IHigh_AVGD_A.png | png file containing a graph of the absolute error on IHigh measure             |
| TestName_AvgNum_IHigh_AVGD_R.svg | svg file containing a graph of the relative error on IHigh measure             |
|   TestName_AvgNum_ILow_AVGD.csv  | csv file containing the data for the error plot on ILow measure                |
|  TestName_AvgNum_ILow_AVGD_A.png | png file containing a graph of the absolute error on ILow measure              |
|  TestName_AvgNum_ILow_AVGD_R.svg | svg file containing a graph of the relative error on ILow measure              |
|  TestName_AvgNum_VHigh_AVGD.csv  | csv file containing the data for the error plot on VHigh measure               |
| TestName_AvgNum_VHigh_AVGD_A.png | png file containing a graph of the absolute error on VHigh measure             |
| TestName_AvgNum_VHigh_AVGD_R.svg | svg file containing a graph of the relative error on VHigh measure             |
|   TestName_AvgNum_VLow_AVGD.csv  | csv file containing the data for the error plot on VLow measure                |
|  TestName_AvgNum_VLow_AVGD_A.png | png file containing a graph of the absolute error on VLow measure              |
|  TestName_AvgNum_VLow_AVGD_R.svg | svg file containing a graph of the relative error on VLow measure              |                                            |

### Custom test
In the Settings.py file, variables can be changed to customize the test. 
The variable and its according description are now listed

| Variable                         | Description                                                  |
|----------------------------------|--------------------------------------------------------------|
| LoadInternalSafetyCurrent_A      | The maximum current the load will accept to sink, in Ampere  |
| LoadInternalSafetyPower_W        | The maximum power the load will accept to sink, in Watt      |
| NumberOfpointsPerDuty            | Number of samples read per duty, same for DMM and SPIN       |
| duty_step_Percent                | The step of duty cycle when we send 'u' (or 'd') to SPIN     |
| starting_duty_cycle_Percent      | The value of duty cycle when we send 'r' to SPIN             |
| AccVHighCalibrationDutyPercent   | The duty cycle for the VHigh measures, in percent            |
| AccVHighCalibrationLowCurrent_mA | The current sink by the load for the VHigh measures, in mA   |
| AccIHighCalibrationDuty_Percent  | The duty cycle value for the IHigh measures, in percent      |
| AccIHighCalibrationVHigh_mV      | The VHigh value for the IHigh measures, in mV                |
| AccVLowCalibrationLowCurrent_mA  | The current sink by the load for the VHLow measures, in mA   |
| AccVLowCalibrationVHigh_mV       | The VHigh value for the VLow measures, in mV                 |
| AccILowCalibrationDuty_Percent   | The duty cycle value for the ILow measures, in percent       |
| AccILowCalibrationVHigh_mV       | The VHigh value for the ILow measures, in mV                 |
| AccNumberOfStepInVLowDutySweep   | The number of step for VLow measures                         |
| AccNumberOfVHighStep             | The number of VHigh steps for VHigh measures                 |
| AccStepMinVHigh_mV               | The value of the first VHigh step, for VHigh measures, in mV |
| AccStepMaxVHigh_mV               | The value of the last VHigh step, for VHigh measures, in mV  |
| AccIHighNumberOfCurrentStep      | The number of IHigh steps for IHigh measures                 |
| AccIHighStepMinCurrent_mA        | The value of the first IHigh step, for IHigh measures, in mA |
| AccIHighStepMaxCurrent_mA        | The value of the last IHigh step, for IHigh measures, in mA  |
| AccILowNumberOfCurrentStep       | The number of ILow steps for ILow measures                   |
| AccILowStepMinCurrent_mA         | The value of the first ILow step, for ILow measures, in mA   |
| AccILowStepMaxCurrent_mA         | The value of the last ILow step, for ILow measures, in mA    |


## Histogram

This script is intended to create an histogram from a great number of measures.

### Custom test

To run this test, you need to choose the parameters you want to apply. 

- You can set the number of samples to be read per duty cycle (DMM and SPIN). The larger the number of points, the more accurate the resulting histogram will be. However, this will extend the test duration.
- You can also adjust the duty cycle for the VHigh measurements and the current sink by the load for the VHigh measures.
By increasing these parameters, you will increase the voltage and current across the LEGs.
- Enventully, you can choose the voltage values at which you want to take your measurements. However, please note that increasing the number of data points will significantly extend the duration of your test.

### Output folder structure
The script generates output in the folder 'Output_Histogram'.

## Launching HMI (Human machine interface)
Run the main_file.py, either with a terminal command (in the folder of the file): ```python main_file.py  ``` or via your favorite IDE (mine is VS CODE) and then open the web page at the given url via your favorite web browser.