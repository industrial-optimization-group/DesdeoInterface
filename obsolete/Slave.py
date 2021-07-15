import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

import pyfirmata
from pyfirmata import Arduino, util
from desdeo_interface.components.Component import Component
from desdeo_interface.components.Button import Button
from desdeo_interface.components.RotaryEncoder import RotaryEncoder
from typing import List
class Slave:
    """
    Attributes:
        _board (Arduino): The microcontroller (arduino) board
        _it (pyfirmata.Iterator): Iterator which updates pin values
        _components  ([Component]): The components connected to the board
    """
    def __init__(
        self,
        port: str,
    ):
        self._board = Arduino(port)
        self._it = util.Iterator(self._board)
        self._it.start()
    
    def find_components(self):
        pass 
