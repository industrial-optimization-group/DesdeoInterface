import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board
import numpy as np
import time # For dynamic step sizes

from typing import List

class RotaryEncoder(Component):
    current_value: float # Value of the encoder
    state: int # The position of the encoder, 1 or 0
    state_prev: int # the previous position of the encoder, 1 or 0

    # Needed for dynamic values: 
    # Record times of each rotations, check which have happened in the last second, calculate rotations per second, adjust step size accordingly
    rotations: List[float] = [] # list of rotation start times


    def __init__(self, board: Board, pins: List[int]):
        if len(pins) != 2:
            raise Exception("Rotary encoder needs 2 digital pins")
        super().__init__(board, pins, True)
        self.state_prev = self.get_pin_values[0] # Initial state
        self.current_value = 0
    
    # TODO Make it so that movements when not asked are not accounted for
    def get_value(self, min: float = -np.inf, max: float = np.inf, step: float = 0.01) -> float:
        """
        Get real time value from the rotary encoder
        Args:
            min (float): Minimun reachable value, defaults to negative infinity
            max (float): Maximun reachabe value, defaults to (positive) infinity
            step (float): Step size of each rotation, defaults to 0.01
        Returns:
            float: the current value from the rotary encoder
        """
        pin0, pin1 = self.get_pin_values()
        self.state = pin0

        # if current state is different than prev state then rotary encoder has moved
        if (pin1 != self.state_prev and pin0 == 1): # Only react when pin0 is equal to one 
            self.rotations.append(time.time()) # Add the rotation time to rotations list, needed for dynamic steps
            self.current_value += self._determine_direction(pin1) * step 
        
        self.state_prev = pin0
        return self.current_value
    
    def get_dynamic_value(self, min: float = -np.inf, max: float = np.inf):
        """
        get the value of the potentiometer with dynamic step sizes, slow turns => smaller steps => more accuracy
        Args:
            min (float): Minimun reachable value, defaults to negative infinity
            max (float): Maximun reachabe value, defaults to (positive) infinity
        Returns:
            float: the current value from the rotary encoder
        """

        # calculate rps
        self.rotations = list(filter(lambda t: t > time.time() - 1)) # Filter out the rotations which happened more than a second ago
        rps = len(self.rotations) # Rotations per second

        # Calculate the step size as a function of rps
        # TODO this function could be a argument and the default needs to be something nicer but i'll think of it later
        step = np.log2(rps + 0.01) 

        return self.get_value(min,max, step)
    
    def _determine_direction(self, pin1):
        """
        Get the rotation direction from the rotary encoder
        Args:
            pin1 (int): the position of the second pin
        Returns:
            int: the direction the rotary encoder has been moved
        """
        return -1 if self.state != pin1 else 1

    
