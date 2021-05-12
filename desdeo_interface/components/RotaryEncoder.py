import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board
import numpy as np

class RotaryEncoder(Component):
    current_value: float #Value of the encoder

    def __init__(self, board: Board, pins: list[int]):
        if len(pins) != 2:
            raise Exception("Rotary encoder needs 2 digital pins")
        super().__init__(board, pins, True)
        self.current_value = 0
    
    def get_value(self, min: float = -np.inf, max: float = np.inf) -> float:
        return 0

    
