import numpy as np
import matplotlib.pyplot as plt
from scipy import signal

# Parameters (Example values, adjust as per your specific system)
V_ref = 25/2  # Voltage reference
duty_ref = 0.5
Kp = 0.2        # Proportional gain of the PI controller
Ki = 0.8       # Integral gain of the PI controller
saturation_limit_up = 0.97  # Saturation limit for duty cycle
saturation_limit_down = 0.03
sampling_time = 0.01  # Sampling time in seconds

# Initialize variables
V1_adc = 0  # Initial ADC value
err = 0
duty_cycle = 0
integral = 0
time = np.arange(0, 20, sampling_time)
closed_loop_duty_cycle = []

# PI controller function
def PI_controller(err, integral, Kp, Ki, sampling_time):
    integral = integral + err * sampling_time
    duty_cycle = Kp * err + Ki * integral
    return duty_cycle, integral

# Duty cycle saturation function
def saturate_duty_cycle(duty_cycle, saturation_limit_up, saturation_limit_down):
    if duty_cycle > saturation_limit_up:
        return saturation_limit_up
    elif duty_cycle < saturation_limit_down:
        return saturation_limit_down
    else:
        return duty_cycle

# Simulation loop
for t in time:
    # Measure the error
    # err = V_ref - V1_adc
    err = duty_ref - duty_cycle
    # print(err)
    # Calculate the duty cycle using PI controller
    duty_cycle, integral = PI_controller(err, integral, Kp, Ki, sampling_time)
    
    
    # Apply saturation
    duty_cycle = saturate_duty_cycle(duty_cycle, saturation_limit_up, saturation_limit_down)
    print(duty_cycle)
    
    # Store the closed-loop duty cycle
    closed_loop_duty_cycle.append(duty_cycle)
    
    # Simulate the system response (Example: simple first-order system)
    V1_adc += duty_cycle * sampling_time

# Plot results
plt.plot(time, closed_loop_duty_cycle, label="Closed-loop Duty Cycle")
plt.xlabel("Time (s)")
plt.ylabel("Duty Cycle")
plt.title("Closed-loop Duty Cycle vs Time")
plt.legend()
plt.grid(True)
plt.show()
