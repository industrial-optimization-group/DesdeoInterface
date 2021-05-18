import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Component import Component

from pyfirmata import Board
import time

from enum import Enum
class Action(Enum):
    CLICK = 1
    DOUBLE_CLICK = 2
    HOLD = 3
    NO_ACTION = 4

# Maybe if the button just had an Action method which would wait for any action and return it
# So it would return None | Click | D_click | Hold
# This way one could maybe expect multiple actions without hickups
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

        clicked = self.pin_values[0] == 1

        if not clicked and self.prev_value:
            self.prev_value = False
            return False

        if clicked and not self.prev_value: #not self.prev_value should take care of accidental "holds" *double confirmations, and debouncing
            self.prev_value = clicked
            return True
        
        return False

    def double_click(self, max_time_between_clicks_ms: float = 250) -> bool:
        """
        is the button clicked twice in a short period a.k.a double click
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
            #print(f"time left: {start_time + hold_time_s - time.time()}", end='\r') # Trying out, seems to work
            if (start_time + hold_time_s <= time.time()): return True # Button has been held for desired hold time
            self.click() # if button is released then prev value = False
        return False
    
    # Maybe
    def action(self, hold_time_ms = 1000, max_time_between_clicks_ms = 250) -> Action:
        hold_time_s = hold_time_ms / 1000
        max_time_between_clicks_s = max_time_between_clicks_ms / 1000
        start_time = time.time()

        first_click = self.click()
        if not first_click: return Action.NO_ACTION

        while (self.prev_value): # Check for holding
            if (start_time + hold_time_s <= time.time()): return Action.HOLD
            self.click() # Check if still holding, if not then look for double click
        
        while (start_time + (max_time_between_clicks_s) > time.time()):
            if self.click(): return Action.DOUBLE_CLICK
        
        return Action.CLICK


# testing the button
if __name__ == "__main__":
    from pyfirmata import Arduino, util
    port = "COM3"  # Serial port the board is connected to
    pin = 2  # Digital pin  the button is connected to
    board = Arduino(port)
    it = util.Iterator(board)
    it.start()
    button = Button(board, pin)
    # Testing action
    clicks_b2b = 0
    while clicks_b2b <= 10: # If 10 clicks back to back quit
        print(button.action())

    # Testing basic methods
    # print("Click the button to continue")
    # while not button.click():
    #     pass
    # print("Button clicked!")

    # print("Click the button again to continue")
    # while not button.click():
    #     pass
    # print("Button clicked!")

    # times = 5
    # print(f"Click the button {times} times to continue")
    # for i in range(times):
    #     print(f"{times - i} left")
    #     while not button.click():
    #         pass
    # print(f"Button clicked {times} times")

    # print("Double click the button")
    # while not button.double_click():
    #     pass
    # print("Button double clicked!")

    # hold_time = 1500
    # print(f"hold the button for {hold_time / 1000} seconds")
    # while not button.hold(hold_time):
    #     pass
    # print("\nButton was held")

    # print("\nTesting hold when also waiting for clicks")
    # while not button.hold(hold_time):
    #     if (button.double_click()): 
    #         print("button doubleclicked")
    #     if (button.click()): # Won't always register click because expecting a doubleclick
    #         print("button clicked\n")

    
    # while not button.hold(hold_time): 
    #     if (button.click()): # Will sometimes register a click even though holding and sometimes not register a click even though just clicking
    #         print("button clicked\n")

    # print("\nButton was held")