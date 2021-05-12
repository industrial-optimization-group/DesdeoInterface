import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board, Pin
import time

class Button(Component):
    """
    A button class to handle input from a connected button
    Args:
        board (pyfirmata.pyfirmata.Board): The board (Arduino) the button is connected to
        pin (int): The digital pin the button is connected to
    Raises:
        #todo interface exceptions?
        Exception: digital pin doesn't exist on the arduino uno or is reserved for system
    """
    prev_value: bool = False
    #debounce_time_ms: float = 50 #milliseconds, Minimun delay between clicks, to avoid unwanted clicks

    def __init__(self, board: Board, pin: int):
        super().__init__(board, [pin], True)

    # Todo function as parameter if needed, callback idea
    def click(self) -> bool:
        """
        is the button clicked
        Returns:
            bool: whether or not the button is clicked
        """
        time.sleep(0.01) #wont work without, because pin is unavailable and pyfirmata stalls or something

        clicked = self.get_pin_values()[0] == 1

        if not clicked and self.prev_value:
            self.prev_value = False
            return False

        if clicked and not self.prev_value: #not self.prev_value should take care of accidental "holds" *double confirmations, and debouncing
            self.prev_value = clicked
            return True
        
        return False

    def double_click(self, max_time_between_clicks_ms: float = 250) -> bool:
        """
        is the button clicked twice in a short period a.k.a double clicked
        Args:
            max_time_between_clicks_ms (float): Maximum time between the clicks in milliseconds, defaults to 250ms
        Returns:
            bool: whether or not the button is double clicked 
        """
        first_click = self.click()
        if not first_click: return False
        start_time = time.time()
        while (start_time + (max_time_between_clicks_ms / 1000) > time.time()): # this will freeze the execution of other program for max_time_between_clicks every time it is called :(
            second_click = self.click() #Can't be holding down, because prev_value takes care of it
            if second_click: return True
        return False

 
    def hold(self, hold_time_ms: float = 1000):
        """
        is the button being held down
        Args: 
            hold_time_s (float): how long should the button be held down
        Returns:
            bool: whether or not the button is being held down
        """
        hold_time_s = hold_time_ms / 1000
        start_time = time.time()
        self.click()
        while (self.prev_value): # Prev value will be false if button has been released
            print(f"time left: {start_time + hold_time_s - time.time()}", end='\r') # Trying out, seems to work
            if (start_time + hold_time_s <= time.time()): return True # Button has been held for desired hold time
            self.click() # if button is released then prev value = False
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
    print("Click the button to continue")
    while not button.click():
        pass
    print("Button clicked!")

    print("Click the button again to continue")
    while not button.click():
        pass
    print("Button clicked!")

    times = 5
    print(f"Click the button {times} times to continue")
    for i in range(times):
        print(f"{times - i} left")
        while not button.click():
            pass
    print(f"Button clicked {times} times")

    print("Double click the button")
    while not button.double_click():
        pass
    print("Button double clicked!")

    hold_time = 1500
    print(f"hold the button for {hold_time / 1000} seconds")
    while not button.hold(hold_time):
        pass
    print("\nButton was held")