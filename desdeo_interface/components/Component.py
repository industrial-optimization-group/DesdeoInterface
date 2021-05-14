from pyfirmata import Board, Pin

class Component:
    """
    A base class for components, maybe this was a bad idea as now this is basically pin class from pyfirmata and some components
    Args:
        board (pyfirmata.Board): The board (Arduino) the component is connected
        pins list[int]: The pins the component is connected to
    Raises:
        Exception: a pin doesn't exist on the arduino uno
    """

    pins: list[Pin]

    def __init__(self, board: Board, pins: list[int], is_digital: bool):
        if is_digital:
            if not self._validate_pins(pins, 2, 13):
                raise Exception("Pin is invalid, should be in the range on 2-13")
            self.pins = list(map(lambda pin: board.get_pin(f'd:{pin}:i'), pins))
        else: #Is analog
            if not self._validate_pins(pins, 0, 6):
                raise Exception("Pin is invalid, should be in the range on 0-5")
            self.pins = list(map(lambda pin: board.get_pin(f'a:{pin}:i'), pins))
    
    def _validate_pins(self, pins: list[int], min: int, max: int) -> bool:
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
        in_range = lambda x: x > max and x < min
        return len(list(filter(in_range, pins))) == 0
    
    def get_pin_values(self):
        """
        Read the values of the pins assigned to the component
        Returns:
            list[float]: value of each pin which is between 0.0 - 1.0 or -1.0 if pin is unavailable
        """
        get_pin_value = lambda pin: pin.read() if pin.read() is not None else -1.0
        return list(map(get_pin_value, self.pins))
