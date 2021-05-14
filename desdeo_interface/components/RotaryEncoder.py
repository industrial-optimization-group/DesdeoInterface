import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board
import numpy as np

class RotaryEncoder(Component):
    current_value: float # Value of the encoder
    state: int # The position of the encoder, 1 or 0
    state_prev: int # the previous position of the encoder, 1 or 0

    def __init__(self, board: Board, pins: list[int]):
        if len(pins) != 2:
            raise Exception("Rotary encoder needs 2 digital pins")
        super().__init__(board, pins, True)
        self.state_prev = self.get_pin_values[0] # Inital state
        self.current_value = 0
    
    # TODO Make it so that movements when not asked are not accounted for
    def get_value(self, min: float = -np.inf, max: float = np.inf, step: float = 1) -> float:
        pin0, pin1 = self.get_pin_values()
        self.state = pin0

        #if current state is different than prev state then rotary encoder has moved
        if (pin1 != self.state_prev and pin0 == 1): #Only react when pin0 is equal to one 
            self.current_value += self._determine_direction(pin1) * step 
        
        self.state_prev = pin0
        return self.current_value
    
    def get_dynamic_value(self):
        """
        get the value of the potentiometer with dynamic step sizes, slow turns => smaller steps => more accuracy
        Args:
            max
            min
            propably something which determines the step sizes
        Returns:
            float: set value of rotary encoder
        """
        return 0
    
    def _determine_direction(self, pin1):
        return -1 if self.state != pin1 else 1

    
