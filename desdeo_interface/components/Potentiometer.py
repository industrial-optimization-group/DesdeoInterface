import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board

class Potentiometer(Component):
    """
    A potentiometer class to handle input from a connected potentiometer
    Args:
        board (pyfirmata.pyfirmata.Board): The board (Arduino) the potentiometer is connected
        pin (int): The analog pin the potentiometer is connected to
    Raises:
        Exception: analog pin doesn't exist on the arduino uno
    """

    def __init__(self, board: Board, pin: int):
        super().__init__(board, pin, False)

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
        value_pin = 0 if self.get_pin_value() is None else self.get_pin_value()
        current_value = (value_pin * (max - min)) + min
        return round(current_value, 3) #Temporary rounding so that printing looks somewhat decent

    #TODO documentation
    def get_value_int(self, min: int = 0, max: int = 1) -> int: #inclusive
        return round(self.get_value(min, max))

    #FIX
    #def get_mode_value(self, it: int):  # should this be in own modue
    #    values = []
    #    for _i in range(it):
    #        values.append(self.get_value())
    #    return mymath.mode(values)


if __name__ == "__main__":
    from pyfirmata import Arduino, util
    import time

    def print_value(val):
        print(f"current value = {val}", end="\r")

    port = "COM3"  # Serial port the board is connected to
    pin = 0  # Analog pin the potentiometer is connected to
    board = Arduino(port)
    it = util.Iterator(board)
    it.start()
    pot = Potentiometer(board, pin)

    print("Turn the potentiometer far right")
    current_value = 0.5
    eps = 0.05

    time.sleep(1) # Reading the pin too quickly will cause the pin to be unavailable and thus return 0 as the value

    while current_value > eps:
        current_value = pot.get_value()
        print_value(current_value)

    print("\nTurn the potentiometer far left")
    while pot.get_value() < 1-eps:
        current_value = pot.get_value()
        print_value(current_value)

    value_min = 0.0
    value_max = 5000.0
    print(f"\n\nPotentiometer values scaled to {value_min} - {value_max}")

    steps = 4
    for i in range(0,steps + 1): 
        print("\n")
        desired_value = (i/steps) * (value_max - value_min)
        delta = (value_max - value_min) / 50 # 1/50 of the range
        print(f"Set the value close to {desired_value}")
        current_value = 0
        while (abs(current_value - desired_value) > delta):
            current_value = pot.get_value(value_min, value_max)
            print_value(current_value)
