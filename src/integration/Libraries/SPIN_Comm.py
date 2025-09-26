####################################################################################
# TWR : File containing functions related to communication with the SPIN           #
# Author : TWR                                                                     #
####################################################################################

#Python modules import
import os, time, sys, serial, Libraries.TempMeas as TempMeas, sys

#Libraries modules import

## Importing files of the project that are in parent directory ##
# Get the directory of the current script
current_script_dir = os.path.dirname(os.path.abspath(__file__))

# Get the parent directory
parent_dir = os.path.dirname(current_script_dir)

# add parent directory to path
sys.path.append(parent_dir)


###################################################################
# \brief Initialize a serial object with timeout.
#
# This function initializes a serial object with specified parameters
# including baudrate, bytesize, parity, stopbits, and timeout.
#
# \param SerialObj The serial object to be initialized.
# \param baudrate The baudrate for serial communication.
# \param bytesize The number of data bits.
# \param parity The parity setting ('N' for None, 'E' for Even, 'O' for Odd).
# \param stopbits The number of stop bits.
# \param timeout_sec The timeout for serial communication in seconds.
###################################################################
def InitSerialObjectTimeout(SerialObj, baudrate, bytesize, parity, stopbits, timeout_sec):
    SerialObj.baudrate = baudrate
    SerialObj.bytesize = bytesize
    SerialObj.parity = parity
    SerialObj.stopbits = stopbits
    SerialObj.timeout = timeout_sec

###################################################################
# \brief Initialize a serial object without timeout.
#
# This function initializes a serial object with specified parameters
# including baudrate, bytesize, parity, and stopbits.
#
# \param SerialObj The serial object to be initialized.
# \param baudrate The baudrate for serial communication.
# \param bytesize The number of data bits.
# \param parity The parity setting ('N' for None, 'E' for Even, 'O' for Odd).
# \param stopbits The number of stop bits.
###################################################################
def InitSerialObjectNoTimeout(SerialObj, baudrate, bytesize, parity, stopbits):
    SerialObj.baudrate = baudrate
    SerialObj.bytesize = bytesize
    SerialObj.parity = parity
    SerialObj.stopbits = stopbits


#----------------------------------------------------------------------------------------------
#               COMMUNICATION PROTOCOL
#----------------------------------------------------------------------------------------------

def SendTwistMessage(SerialObj, Message, delay=0.1):
    """
    Send a message via serial communication.

    Args:
        SerialObj: The serial object used for communication.
        Message (str): The message to be sent.

    Returns:
        None
    """
    # Define the chunk size
    # if not isinstance(Message, str):
    #     Message = str(Message)
    chunk_size = 10

    # Calculate the number of chunks
    # print(type(Message))

    num_chunks = (len(Message) + chunk_size - 1) // chunk_size

    # Send each chunk
    for i in range(num_chunks):
        # Calculate the start and end indices for the current chunk
        start_index = i * chunk_size
        end_index = min((i + 1) * chunk_size, len(Message))

        # Extract the current chunk
        chunk = Message[start_index:end_index]
        # print(chunk)
        # Send the chunk via serial
        # print(chunk.encode('utf-8'))
        # SerialObj.write(chunk.encode('utf-8'))

        try:
            # Envoyer la commande
            SerialObj.write(chunk.encode('utf-8'))  
        except Exception as e:
            print(f"Error occurred: {e}")

    # Send the end of line
    SerialObj.write(b'\r\n')
    time.sleep(delay)  # Ajuster le délai en fonction de la latence du microcontrôleur

    # SerialObj.write(Message.encode('utf-8'))  # Convert the string message to bytes and send it via serial
    # time.sleep(0.5)
    # SerialObj.write(b'2355\r\n')  # waits a little and sends the end of line



def SendCommand(SerialObj, action, *args, delay=0.2, verbose = False, send_mes_delay = 0.1):
    """
    Generate a message for the Twist board based on the action and optional parameters
    and send it via serial communication.

    Args:
        action (str): The action to perform (e.g., "IDLE", "POWER", "CAPACITOR", "SERIAL", "CALIBRATE").
        delay(float) = the time to wait after sending the Command
        verbose(bool): True to print the output command
        send_mes_delay(float): Time for the low-level driver to write the message on the serial port
        *args: Optional arguments corresponding to the action.

    Returns:
        str: The generated message for the Twist board.
    """

    action_types = ("LEG", "CAPA", "DRIVER", "BUCK", "BOOST", "REFERENCE", "DUTY", "CALIBRATE", "TEST_SENSI")

    # Dictionary mapping actions to their message formats
    message_formats = {
        "IDLE": "d_i",
        "POWER_OFF": "d_f",
        "POWER_ON": "d_o",
        "READ" : "o_r",
        "ENABLE" : "o_a",
        "LEG": lambda leg, state: f"s_{leg.upper()}_l_{state.lower()}",
        "CAPA": lambda leg, state: f"s_{leg.upper()}_c_{state.lower()}",
        "DRIVER": lambda leg, state: f"s_{leg.upper()}_v_{state.lower()}",
        "BUCK": lambda leg, state: f"s_{leg.upper()}_b_{state.lower()}",
        "BOOST": lambda leg, state: f"s_{leg.upper()}_t_{state.lower()}",
        "REFERENCE": lambda leg, variable, value: f"s_{leg.upper()}_r_{variable.upper()}_{value:.5f}",
        "DUTY": lambda leg, value: f"s_{leg.upper()}_d_{value:.5f}",
        "CALIBRATE": lambda variable, gain, offset: f"k_{variable.upper()}_g_{gain:.8f}_o_{offset:.8f}",
        "TEST_SENSI": lambda leg, ref: f"t_{leg.upper()}_r_{ref:.1F}",
        }

    # Check if action is valid
    if action not in message_formats:
        raise ValueError(f"Invalid action: {action}")

    # Generate message based on action and arguments
    message_out = message_formats[action](*args) if action in action_types else message_formats[action]

    # Send the generated message via serial communication
    SendTwistMessage(SerialObj, message_out, delay=send_mes_delay)

    time.sleep(delay)
    if verbose == True: print(message_out) 

    return message_out

def ScopeMimicryCommand(SerialObj, command):
    SerialObj.write(command.encode('utf-8'))
    # Wait for a response (optional)
    time.sleep(1)
    # Read the response (optional)
    response = SerialObj.read_all().decode()
    print(f"Response: {response}")

def getLine(SerialObj, split_character = ':'):
        """
        Retrieves the next serial line from the TWIST in the buffer and split it along the current characters.

        Parameters:
            - self: Instance of the class containing the measurement methods.
            - split_character: The character to be used to split the line. By default it is ':'.

        Returns:
            - The whole line split along the character.
        """
        reading = SerialObj.readline().decode('utf-8').split(split_character)

        return reading
