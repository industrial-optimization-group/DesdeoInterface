from pyfirmata import Arduino, util
import pyfirmata
from pyfirmata.pyfirmata import Board
from desdeo_interface.components.Button import Button
from desdeo_interface.components.RotaryEncoder import RotaryEncoder
from typing import List

"""
Master component of the physical interface. All other components will connect to the master.
The master will then be passed to an interface class.
"""

class Master:
    _board: Arduino
    _it: util.Iterator
    confirm_button: Button
    decline_button: Button
    wheel: RotaryEncoder
    components: dict # Dictionary of all the components connected to the master

    def __init__(
        self,
        port: str,
        confirm_button_pin: int,
        decline_button_pin: int,
        wheel_pins: List[int],
    ):
        self._board = Board(port)
        self._it = util.Iterator(self._board)
        self._it.start()
        self.confirm_button = Button(self._board, confirm_button_pin)
        self.decline_button = Button(self._board, decline_button_pin)
        self.wheel = RotaryEncoder(self._board, wheel_pins)
        
