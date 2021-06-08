import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board
import numpy as np
import time # For dynamic step sizes

from typing import List

class RotaryEncoder(Component):
    _current_value: float # Value of the encoder
    #state: int # The position of the encoder, 1 or 0
    state_prev: int # the previous position of the encoder, 1 or 0

    # Needed for dynamic values: 
    # Record times of each rotations, check which have happened in the last second, calculate rotations per second, adjust step size accordingly
    rotations: List[float] = [] # list of rotation start times


    def __init__(self, board: Board = None, pins: List[int] = None):
        # if len(pins) != 2:
        #     raise Exception("Rotary encoder needs 2 digital pins")
        # super().__init__(board, pins, True)
        super().__init__()
        self.state_prev = 0#self.pin_values[0] # Initial state
        self.current_value = 0
    
    # TODO Make it so that movements when not asked are not accounted for
    def get_value(self, min: float = None, max: float = None, step: float = 0.01) -> float:
        """
        Get the value from the rotary encoder
        Args:
            min (float): Minimun reachable value, defaults to negative infinity
            max (float): Maximun reachabe value, defaults to (positive) infinity
            step (float): Step size of each rotation, defaults to 0.01
        Returns:
            float: the current value from the rotary encoder
        """
        if (min is None) != (max is None): return # Xor: return if only one is None
        if min is None and max is None: 
            min = -np.inf
            max = np.inf

        elif (min >= max): return
        
        pin0, pin1 = self.pin_values
        if pin0 is None or pin1 is None: self.current_value
        
        # if current state is different than prev state then rotary encoder has moved
        if (pin0 != self.state_prev): # Add pin0 == 0 if crowtail 2.0 encoder else remove
            self.rotations.append(time.time()) # Add the rotation (time) to rotations list, needed for dynamic steps
            self.current_value += self._determine_direction(pin0, pin1) * step 
            # Make sure the values don't exceed bounds: Rather make them loop
            if self.current_value > max: self.current_value = min
            elif self.current_value < min: self.current_value = max
        
        self.update_rotations_list()
        self.state_prev = pin0
        return self.current_value
    
    def get_dynamic_value(self, min: float = -np.inf, max: float = np.inf, step = 0.01):
        """
        get the value of the potentiometer with dynamic step sizes, slow turns => smaller steps => more accuracy
        Args:
            min (float): Minimun reachable value, defaults to negative infinity
            max (float): Maximun reachabe value, defaults to (positive) infinity
            step (float): Base step value when at 'normal' rotation speeds
        Returns:
            float: the current value from the rotary encoder
        """

        rps = len(self.rotations) # Rotations per second

        # According to my large-scale testing:
        # rps values close to 8 are the values you get when turning the encoder at normal speeds
        # 1-4 are slow and anything above 15 is quite fast at best reaching 35-40 rps
        # This all might change when there is a shell over the encoder

        # Calculate the step size as a function of rps
        # TODO this function could be a argument and the default needs to be something nicer but i'll think of it later
        step = ((rps + 1)/8.) * step
        step = step * 2

        return self.get_value(min, max, step)
    
    def _determine_direction(self, pin0, pin1):
        """
        Get the rotation direction from the rotary encoder
        Args:
            pin1 (int): the position of the second pin
        Returns:
            int: the direction the rotary encoder has been moved
        """
        return -1 if pin0 != pin1 else 1
    
    def update_rotations_list(self):
        # Filter out the rotations which happened more than a second ago
        current = time.time() - 1
        self.rotations = list(filter(lambda t: t > current, self.rotations)) 
    
    @property
    def current_value(self):
        return self._current_value

    @current_value.setter
    def current_value(self, value: float):
        self._current_value = value
    
    
        


if __name__ == "__main__":
    from pyfirmata import Arduino, util
    port = "COM3"  # Serial port the board is connected to
    pins = [8,9]  # pins the rotary encoder is connected
    board = Arduino(port)
    it = util.Iterator(board)
    it.start()
    rot_enc = RotaryEncoder(board, pins)
    print("Enter a value greater than 100 to stop")
    while True:
        value = rot_enc.get_dynamic_value(step = 1)
        print(f"current value: {value}", end="\r")
        if (value > 100): break