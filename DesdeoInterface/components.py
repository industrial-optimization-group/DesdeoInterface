from pyfirmata import Arduino, util, Board, Pin
import time


"""
Physical components used by physical interfaces
"""

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
    prev_value: bool

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


class Button:
    """
    A button class to handle input from a connected button
    Args:
        board (pyfirmata.pyfirmata.Board): The board (Arduino) the button is connected to
        pin (int): The digital pin the button is connected to
    Raises:
        #todo interface exceptions?
        Exception: digital pin doesn't exist on the arduino uno or is reserved for system
    """
    pin: Pin
    delay_time_ms: float = 150 #milliseconds, Minimun delay between clicks, to avoid accidental clicks 

    def __init__(self, board: Board, pin: int):
        if pin < 2 or pin > 13:
            raise Exception("Pin is invalid, should be in the range of 2-15")
        self.pin = board.get_pin(f'd:{pin}:i')
        self.delay_time_s = self.delay_time_ms / 1000

    # Todo function as parameter if needed, callback idea
    def click(self) -> bool:
        """
        is the button clicked
        Returns:
            bool: whether or not the button is clicked
        """
        time.sleep(0.01) #wont work without, because pin is unavailable and pyfirmata stalls or something

        clicked = self.pin.read() == 1

        if clicked:
            time.sleep(self.delay_time_s)
            return True
        return False

    # todo
    def double_click(self, max_time_between_clicks_ms: float = 750) -> bool:
        return False


    def hold(self, hold_time_ms: float = 1000):
        """
        is the button being held down
        args: 
        hold_time_s (float): how long should the button be held down
        Returns:
            bool: whether or not the button is being held down
        """
        holding = self.click()
        time_holding_ms = self.delay_time_ms
        while (holding):
            if (time_holding_ms >= hold_time_ms): return True 
            holding = self.click()
            time_holding_ms += self.delay_time_ms #I dont think this is very efficient or accurate, but it gets the job kinda done
        return False

        



# Todo some tests, currently simple test for button
if __name__ == "__main__":
    port = "COM3"  # Serial port the board is connected to
    pin = 2  # Digital pin  the button is connected to
    board = Arduino(port)
    it = util.Iterator(board)
    it.start()
    button = Button(board, pin)
    print("Press the button to continue")
    while not button.click():
        pass
    print("Button clicked!")
    print("hold the button")
    while not button.hold():
        pass
    print("Button was held")

