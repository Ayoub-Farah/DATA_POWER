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
@author Guillaume Arthaud 
@author Thomas Walter 
"""

#Python modules import
import inspect
import struct
import time, serial

class Shield_Device:
    def __init__(self, shield_port, shield_type = "TWIST", baudrate = 115200, bytesize = 8, parity = "N", stopbits = 1, timeout_sec = 2, product_id = 0x0101, vendor_id = 0x2fe3):

        self.shield_serialObj = serial.Serial(port = shield_port)
        self.CommPortDescWithoutPort = shield_type
        self.shield_type = shield_type

        self.baudRate = baudrate
        self.bytesize = bytesize
        self.parity = parity
        self.stopbits = stopbits
        self.Timeout_s = timeout_sec
        self.shield_pid = product_id
        self.shield_vid = vendor_id

        self.shield_serialObj.baudrate = baudrate
        self.shield_serialObj.bytesize = bytesize
        self.shield_serialObj.parity = parity
        self.shield_serialObj.stopbits = stopbits
        self.shield_serialObj.timeout = timeout_sec

        twist_message_length = 16
        twist_message_index = { "D1": {"index": 0},
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


        ownverter_message_length = 21
        ownverter_message_index = { "D1": {"index": 0},
                                    "V1": {"index": 1},
                                    "I1": {"index": 2},
                                    "M1": {"index": 3},
                                    "T1": {"index": 4},
                                    "D2": {"index": 5},
                                    "I2": {"index": 6},
                                    "V2": {"index": 7},
                                    "M2": {"index": 8},
                                    "T2": {"index": 9},
                                    "D3": {"index": 10},
                                    "I3": {"index": 11},
                                    "V3": {"index": 12},
                                    "M3": {"index": 13},
                                    "T3": {"index": 14},
                                    "VH": {"index": 15},
                                    "IH": {"index": 16},
                                    "AN": {"index": 17},
                                    "CE": {"index": 18},
                                    "CR": {"index": 19},
                                    "RS": {"index": 20}}

        if self.shield_type == "TWIST" :
            self.shield_message_index = twist_message_index
            self.message_lenght = twist_message_length

        else :
            self.shield_message_index = ownverter_message_index
            self.message_lenght = ownverter_message_length


    def setSerialPort(self, port):
        self.CommPort = port

    def setVendorID(self, vendor_id):
        self.shield_pid = vendor_id

    def setProductID(self, product_id):
        self.shield_vid = product_id

    def getSerialObjID(self):
        return self.shield_serialObj

    def get_supported_measurements(self):
        """Return the tuple of measurement identifiers exposed by the shield."""
        return tuple(self.shield_message_index.keys())


    def sendMessage(self, Message):
        """Transmit *Message* over the shield serial link in one burst."""

        if not Message:
            return

        if Message.endswith("\r") or Message.endswith("\n"):
            payload = Message.encode("utf-8")
        else:
            payload = (Message + "\r\n").encode("utf-8")

        self.shield_serialObj.write(payload)
        self.shield_serialObj.flush()


    def wait_for_scope_record(self, timeout=20.0):
        """Wait for a scope record dump and decode it into structured samples.

        Parameters
        ----------
        timeout: float, optional
            Maximum number of seconds to wait for the full capture.  The timer
            covers the entire transaction between the ``begin record`` and
            ``end record`` markers.

        Returns
        -------
        dict
            A dictionary with the following keys:

            ``columns``
                Tuple of column names advertised in the record header.

            ``index``
                Optional integer index reported by the firmware (``None`` if
                missing).

            ``samples``
                List of dictionaries, each mapping a column name to the float
                value decoded for a single sample.

            ``raw``
                The raw hexadecimal payload lines as emitted by the firmware.

        Raises
        ------
        TimeoutError
            If the record terminator is not received before *timeout*
            elapses.
        RuntimeError
            If the capture is malformed (missing header or inconsistent
            payload size).
        """

        deadline = time.time() + timeout
        serial_obj = self.shield_serialObj
        in_record = False
        header_lines = []
        payload_lines = []

        while time.time() < deadline:
            raw_line = serial_obj.readline()
            if not raw_line:
                continue

            try:
                line = raw_line.decode("ascii", errors="ignore").strip()
            except UnicodeDecodeError:
                # Skip undecodable garbage and keep listening.
                continue

            if not line:
                continue

            if line == "begin record":
                in_record = True
                header_lines.clear()
                payload_lines.clear()
                continue

            if not in_record:
                # Ignore any other console output.
                continue

            if line == "end record":
                return self._parse_scope_record(header_lines, payload_lines)

            if line.startswith("#") and len(header_lines) < 2:
                header_lines.append(line)
                continue

            payload_lines.append(line)

        raise TimeoutError("Timed out waiting for the scope record to finish.")


    @staticmethod
    def _parse_scope_record(header_lines, payload_lines):
        if not header_lines:
            raise RuntimeError("Scope record did not include a header line.")

        header = header_lines[0].lstrip("#").strip()
        columns = [token.strip() for token in header.split(",") if token.strip()]

        record_index = None
        if len(header_lines) > 1:
            candidate = header_lines[1].lstrip("#").strip()
            if candidate:
                try:
                    record_index = int(candidate)
                except ValueError:
                    # Some firmwares reuse this line for additional headers.
                    pass

        if not columns:
            raise RuntimeError("Scope record header did not expose any columns.")

        floats = []
        for entry in payload_lines:
            try:
                floats.append(struct.unpack('>f', bytes.fromhex(entry))[0])
            except (ValueError, struct.error):
                # Ignore malformed entries but keep track of the raw payload.
                continue

        stride = len(columns)
        if stride == 0 or len(floats) < stride:
            samples = []
        else:
            trimmed_length = len(floats) - (len(floats) % stride)
            samples = [
                {
                    column: floats[offset + idx]
                    for idx, column in enumerate(columns)
                }
                for offset in range(0, trimmed_length, stride)
            ]

        return {
            "columns": tuple(columns),
            "index": record_index,
            "samples": samples,
            "raw": tuple(payload_lines),
        }


    def getMeasurement(self, measurement_type):
        """
        Retrieve the latest measurement published by the shield for the given
        ``measurement_type``. This method must be called while the converter is
        in ``POWER ON`` so that telemetry frames are streamed on the serial
        link.

        Parameters
        ----------
        measurement_type: str
            Identifier of the measurement to read (for example ``"V1"`` or
            ``"IH"``). The list of supported identifiers depends on the shield
            family that was selected when instantiating :class:`Shield_Device`.

        Returns
        -------
        float
            The value parsed from the latest telemetry frame for the requested
            measurement.

        Raises
        ------
        ValueError
            If ``measurement_type`` is not part of the measurements advertised
            by the current shield.
        """
        if measurement_type not in self.shield_message_index:
            supported = ", ".join(sorted(self.shield_message_index.keys()))
            raise ValueError(
                "Invalid measurement type for {shield}. Supported types are: {supported}."
                .format(shield=self.shield_type, supported=supported)
            )

        # time.sleep(self.short_delay)
        index_meas = self.shield_message_index[measurement_type]["index"]

        self.shield_serialObj.reset_input_buffer()

        size_check = False
        while size_check == False:
            reading = self.getLine()
            reading = [elem.replace('{', '').replace('}', '') for elem in reading]  #eliminates curly brackets from the message
            if len(reading) == self.message_lenght : size_check = True              #tests that the buffer has the right length

        return float(reading[index_meas])

    def getLine(self, split_character = ':'):
        """
        Retrieves the next serial line from the TWIST in the buffer and split it along the current characters.

        Parameters:
            - self: Instance of the class containing the measurement methods.
            - split_character: The character to be used to split the line. By default it is ':'.

        Returns:
            - The whole line split along the character.
        """
        reading = self.shield_serialObj.readline().decode('utf-8').split(split_character)

        return reading


    def sendCommand(self, action, *args, delay=0.2):
        """
        Generate and send a message to the Twist board based on the specified action and optional parameters.

        This method generates a message for the Twist board based on the provided action and optional arguments,
        and sends it via serial communication.

        Args:
            action (str): The action to perform. Supported actions include:
                - "IDLE": Puts the Twist board into an idle state.
                - "POWER_OFF": Turns off power to the Twist board.
                - "POWER_ON": Turns on power to the Twist board.
                - "LEG": Controls the state of a leg on the Twist board.
                - "CAPA": Controls the state of a capacitor on the Twist board.
                - "DRIVER": Controls the state of a driver on the Twist board.
                - "BUCK": Controls the state of a buck converter on the Twist board.
                - "BOOST": Controls the state of a boost converter on the Twist board.
                - "REFERENCE": Sets the reference value for a specific variable on the Twist board.
                - "DUTY": Sets the duty cycle value for a specific leg on the Twist board.
                - "CALIBRATE": Calibrates a specific variable on the Twist board.
                - "TEST_SENSI": Launches the step-response sensitivity test on the selected leg.
                - "TEST_CHIRP": Launches a chirp excitation on the selected leg.
                - "TEST_FIXED_FREQ": Runs a fixed-frequency sinusoidal excitation on the selected leg.
                - "TEST_WAVE_STOP": Stops any ongoing automated excitation (step, chirp or fixed frequency).
            *args: Optional arguments corresponding to the action.
            delay (float, optional): The delay (in seconds) after sending the command. Default is 0.2 seconds.

        Returns:
            str: The generated message sent to the Twist board.

        Raises:
            ValueError: If an invalid action is provided.

        Example:
            To set leg A to ON:
            >>> sendCommand("LEG", "A", "ON")
        """

        # Dictionary mapping actions to their message formats
        message_formats = {
            "IDLE": "d_i",
            "POWER_OFF": "d_f",
            "POWER_ON": "d_o",
            "LEG": lambda leg, state: f"s_{leg.upper()}_l_{state.lower()}",
            "CAPA": lambda leg, state: f"s_{leg.upper()}_c_{state.lower()}",
            "DRIVER": lambda leg, state: f"s_{leg.upper()}_v_{state.lower()}",
            "BUCK": lambda leg, state: f"s_{leg.upper()}_b_{state.lower()}",
            "BOOST": lambda leg, state: f"s_{leg.upper()}_t_{state.lower()}",
            "REFERENCE": lambda leg, variable, value: f"s_{leg.upper()}_r_{variable.upper()}_{value:.5f}",
            "DUTY": lambda leg, value: f"s_{leg.upper()}_d_{value:.5f}",
            "CALIBRATE": lambda variable, gain, offset: f"k_{variable.upper()}_g_{gain:.8f}_o_{offset:.8f}",
            "TEST_SENSI": lambda leg, value: f"t_{leg.upper()}_r_{value:.1f}",
            "TEST_CHIRP": (
                lambda leg,
                       f_start,
                       f_end,
                       duration,
                       amplitude=0.4,
                       offset=0.5,
                       loop=0: (
                    f"t_{leg.upper()}_c_{f_start:.2f}_{f_end:.2f}_{duration:.3f}_{amplitude:.3f}_{offset:.3f}_{int(loop)}"
                )
            ),
            "TEST_FIXED_FREQ": (
                lambda leg, frequency, amplitude=0.4, offset=0.5: (
                    f"t_{leg.upper()}_f_{frequency:.2f}_{amplitude:.3f}_{offset:.3f}"
                )
            ),
            "TEST_WAVE_STOP": lambda leg: f"t_{leg.upper()}_s",
            }

        # Check if action is valid
        if action not in message_formats:
            raise ValueError(f"Invalid action: {action}")

        formatter = message_formats[action]
        if callable(formatter):
            try:
                message = formatter(*args)
            except TypeError as exc:
                signature = inspect.signature(formatter)
                raise ValueError(
                    f"Action {action} expects arguments matching {signature}, received {args}."
                ) from exc
        else:
            if args:
                raise ValueError(
                    f"Action {action} does not take any arguments. Received unexpected values: {args}."
                )
            message = formatter

        # Send the generated message via serial communication
        self.sendMessage(message)
        time.sleep(delay)

        return message