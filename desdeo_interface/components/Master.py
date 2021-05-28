import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

import pyfirmata
from pyfirmata import Arduino, util
from desdeo_interface.components.Component import Component
from desdeo_interface.components.Button import Button
from desdeo_interface.components.RotaryEncoder import RotaryEncoder
from typing import List

"""
Master component of the physical interface.
The master component handles connecting other components? and
selecting values and confirm dialogs
All other components will connect to the master.
The master will then be passed to an interface class.
"""

# How to determine which component (Button, pot, something else) a board has?
#   -With current components: button, pot, rotary encoder: each has a different pin setup
#                           1 dig    1 an   2 dig
#   - Add identifier in sketch so one can read it when connected
#       - 3 almost identical sketches...
#
class Master:
    """
    Attributes:
        _board (Arduino): The microcontroller (arduino) board
        _it (pyfirmata.Iterator): Iterator which updates pin values
    """
    confirm_button: Button
    decline_button: Button
    wheel: RotaryEncoder
    _board: Arduino
    _it: util.Iterator
    #components: dict # Dictionary (or maybe a list) of all the components/microcontrollers connected to the master

    def __init__(
        self,
        port: str = None,
        confirm_button_pin: int = 3,
        decline_button_pin: int = 2,
        wheel_pins: List[int] = [8,9],
    ):
        self._board = Arduino(port) if port else self._find_board()
        self._it = util.Iterator(self._board)
        self._it.start()

        self.confirm_button = Button(self._board, confirm_button_pin)
        self.decline_button = Button(self._board, decline_button_pin)
        self.wheel = RotaryEncoder(self._board, wheel_pins)
        
    def confirm(self):
        while True:
            if self.confirm_button.click(): return True
            if self.decline_button.click(): return False
    
    def select(self, min, max):
        return self.wheel.get_value(min, max, 1)
    
    # Is this fine?
    def _find_board(self) -> Arduino:
        """
        Looks for an arduino board with 
        pyfirmata firmware in it connected to a
        serial port

        Returns:
            Arduino: First, as in lowest serial port index,
            connected arduino board with pyfirmata firmware
        
        Throws:
            Exception: If no board was found
        """
        for i in range(2, 15): # Check serial ports 2-15
            try: 
                b = Arduino(f"COM{i}")
                if b.firmware is not None and "Firmata" in b.firmware:
                    print(f"Connected to board on port COM{i}")
                    return b
                else: b.exit()
            except pyfirmata.serial.SerialException:
                pass
        raise Exception("No board found")
 
    @property
    def board(self):
        return self._board
    
    def connect_components(self):
        """
        Find all boards, each be connected with 
        usb to pc or wire to master or radio to master or wireless to pc...?
        -> Determine component type 
        -> Initialize component
        -> Connect ?add to list? 
        """


if __name__ == "__main__":
    master = Master() # Check connecting
    master2 = Master()
    value = 0
    print("Select 15")
    while value != 15:
        value = master.select(-15, 15)
        print(value, end="\r")
    
    print("Do you like cats?")
    likes_cats = master.confirm()
    print("Awww") if likes_cats else print("Meh")
