from pyfirmata import Board, Pin
import time

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
    from pyfirmata import Arduino, util
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