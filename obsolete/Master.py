import os, sys
from desdeo_mcdm import interactive
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

class Master:
    """

    """
    confirm_button: Button
    decline_button: Button
    wheel: RotaryEncoder
    #_board: Arduino
    #_it: util.Iterator
    #components: dict # Dictionary (or maybe a list) of all the components/microcontrollers connected to the master

    def __init__(
        self,
        confirm_button = None,
        decline_button = None,
        wheel = None,
        #port: str = None,
        #confirm_button_pin: int = 3,
        #decline_button_pin: int = 2,
        #wheel_pins: List[int] = [8,9],
    ):
        # self._board = Arduino(port) if port else self._find_board()
        # self._it = util.Iterator(self._board)
        # self._it.start()

        self.confirm_button = Button() # confirm_button #Button(self._board, confirm_button_pin)
        self.decline_button = Button() #decline_button #Button(self._board, decline_button_pin)
        self.wheel = RotaryEncoder() #wheel #RotaryEncoder(self._board, wheel_pins)
        
    def confirm(self):
        while True:
            if self.confirm_button.click(): return True
            if self.decline_button.click(): return False
    
    def select(self, min, max):
        return self.wheel.get_value(min, max, integer_values=True)
 
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
