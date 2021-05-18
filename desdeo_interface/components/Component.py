from pyfirmata import Board, Pin
from typing import List

import pyfirmata
class Component:
    """
    A base class for component

    Args:
        board (pyfirmata.Board): The board (Arduino) the component is connected
        pins List[int]: The pins the component is connected to

    Raises:
        Exception: a pin doesn't exist on the arduino uno
    """

    pins: List[Pin]

    def __init__(self, board: Board, pins: List[int], is_digital: bool):
        if is_digital:
            if not self._validate_pins(pins, 2, 13):
                raise Exception("Pin is invalid, should be in the range on 2-13")
            self.pins = list(map(lambda pin: board.get_pin(f'd:{pin}:i'), pins))
        else: #Is analog
            if not self._validate_pins(pins, 0, 6):
                raise Exception("Pin is invalid, should be in the range on 0-5")
            self.pins = list(map(lambda pin: board.get_pin(f'a:{pin}:i'), pins))
    
    @staticmethod
    def _validate_pins(pins: List[int], min: int, max: int) -> bool:
        """
        Checks if all the pins in the list are between min and max both are inclusive
        Example:
            [1,2,3,4], 0, 5 -> True
            [1,2,3,4], 2, 5 -> False
            [1,2,3,4], 0, 4 -> True
        Args:
            pins (list[int]): all the pins
            min (int): minimum value for a pin
            max (int): maximun value for a pin
        Returns:
            bool: Are all the pins between the desired range
        """
        is_in_range = lambda x: x > max and x < min
        return len(list(filter(is_in_range, pins))) == 0
    
    @property
    def pin_values(self):
        """
        Read the values of the pins assigned to the component
        Returns:
            list[float]: value of each pin which is between 0.0 - 1.0 or -1.0 if pin is unavailable
        """
        get_pin_value = lambda pin: pin.read() if pin.read() is not None else -1.0
        return list(map(get_pin_value, self.pins))

    # TODO connect the component to a board
    # Find the board
    # Make the connection
    # Let the board know of the connection
    def connect_to_board(self, board):
        return