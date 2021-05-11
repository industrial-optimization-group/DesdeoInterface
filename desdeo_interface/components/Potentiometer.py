from pyfirmata import Board, Pin
import time

# Todo? abstract class for component: __init__ is almost same for each, get_value and such

class Potentiometer:
    """
    A potentiometer class to handle input from a connected potentiometer
    Args:
        board (pyfirmata.pyfirmata.Board): The board (Arduino) the potentiometer is connected
        pin (int): The analog pin the potentiometer is connected to
    Raises:
        Exception: analog pin doesn't exist on the arduino uno
    """
    pin: Pin

    def __init__(self, board: Board, pin: int):
        if pin < 0 or pin > 5:
            raise Exception("Pin is invalid, should be in the range on 0-5")
        self.pin = board.get_pin(f'a:{pin}:i')

    #todo int values, as arg and return? not needed?
    def get_value(self, min: float = 0, max: float = 1) -> float:
        """
        Reads the value of the analog pin the potentiometer is connected and scales it
        Args:
            min (float): Minimum value for the scaling, defaults to 0
            max (float): Maximum value for the scaling, defaults to 1
        Returns:
            Value from the analog pin scaled to desired range
        Raises:
            Exception: min is higher or equal to max
        """
        if max - min <= 0: 
            raise Exception("Min value must be lower than max value!")
        value_pin = 0 if self.pin.read() is None else self.pin.read()
        self.current_value = (value_pin * (max - min)) + min
        return round(self.current_value,3) #Temporary rounding so that printing looks somewhat decent

    def get_value_int(self, min: int = 0, max: int = 1) -> int: #inclusive
        return round(self.get_value(min, max))

    #FIX
    #def get_mode_value(self, it: int):  # should this be in own modue
    #    values = []
    #    for _i in range(it):
    #        values.append(self.get_value())
    #    return mymath.mode(values)

#TODO TESTS