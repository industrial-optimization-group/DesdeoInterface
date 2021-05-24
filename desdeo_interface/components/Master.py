from desdeo_interface.components.Component import Component
from pyfirmata import Arduino, util
from pyfirmata.pyfirmata import Board
from desdeo_interface.components.Button import Button
from desdeo_interface.components.RotaryEncoder import RotaryEncoder
from typing import List

"""
Master component of the physical interface.
The master component handles connecting other components and
selecting values and confirm dialogs
All other components will connect to the master.
The master will then be passed to an interface class.
"""

class Master:
    """
    Attributes:
        _board (pyfirmata.Board): The microcontroller (arduino) board
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
        port: str = "COM3",
        confirm_button_pin: int = 3,
        decline_button_pin: int = 2,
        wheel_pins: List[int] = [8,9],
    ):
        self._board = Arduino(port)
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
    
    @property
    def board(self):
        return self._board


