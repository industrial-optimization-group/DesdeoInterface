import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.components.Button import Button
from desdeo_interface.components.Potentiometer import Potentiometer
from desdeo_interface.components.RotaryEncoder import RotaryEncoder
from desdeo_interface.components.Component import Component

from pyfirmata import Arduino, util
from time import sleep
import numpy as np
from typing import Union, Optional, List, Tuple

class InterfaceException(Exception):
    """
    Raised when an exception related to an interface is encountered.
    """

    pass


# Each component will have it's own microcontroller
# First component to be connected to pc will be the "master" which is actually connected to the pc by usb or some other way, other components will be connected to this "master" by wires\bt\wifi etc
# I guess the master will act as an "interface" which handles scaling, exceptions and so on
# When I'll have a second microcontroller I'll try this connecting and modify the interface accordingly 

# Modifications for the future:
# A component could initialize the interface and others could then connect to it
class Interface:
    """
    A interface class to handle the Decision Maker's inputs given with a physical interface (Arduino)
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.ndarray, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.ndarray, List[int]]): analog pins that are connected to potentiometers, count should be the same as variables
        rotary_encoder_pins (Union[np.ndarray, List[List[int]]]): pairs of digital pins that are connected to rotary encoders
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    Attributes:
        _board (pyfirmata.Board): The microcontroller (arduino) board
        _it (pyfirmata.Iterator): Iterator which updates pin values
    Raises:
        InterfaceException: more variables than potentiometers, cant adjust each variable
        InterfaceException: Has less than two buttons.
        SerialException from pyfirmata: Can't open serialport. Already in use or not plugged
    """

    _board: Arduino
    _it: util.Iterator
    buttons: list[Button]
    potentiometers: list[Potentiometer]
    rotary_encoders: list[RotaryEncoder]
    variable_bounds: Optional[np.ndarray]

    #Todo default values
    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]] = [],
        potentiometer_pins: Union[np.array, List[int]] = [],
        rotary_encoders_pins: Union[np.ndarray, List[List[int]]] = [],
        variable_bounds: Optional[np.ndarray] = None, # [k][0] ideal, [k][1] nadir
    ):
        if variable_bounds is not None:
            if len(variable_bounds) > len(potentiometer_pins) + len(rotary_encoders_pins): # TODO move to validation when migrating to "texas"-model
                raise InterfaceException(
                    "Not enough potentiometers!"
                )
        if len(button_pins) < 3:
            raise InterfaceException("A physical interface requires at least three buttons")

        self._board = Arduino(port)
        self._it = util.Iterator(self._board)
        self._it.start()

        self.buttons = list(map(lambda pin: Button(self._board, pin), button_pins))
        self.potentiometers = list(
            map(lambda pin: Potentiometer(self._board, pin), potentiometer_pins)
        )
        self.rotary_encoders = list(map(lambda pins: RotaryEncoder(self._board, pins), rotary_encoders_pins))

        if variable_bounds is not None:
            self.variable_bounds = variable_bounds
            self.potentiometers = self.potentiometers[:len(variable_bounds)] # Cut off unneeded potentiometers
        else:
            k = len(potentiometer_pins)
            self.variable_bounds = np.column_stack((np.zeros(k), np.ones(k))) # Ideal, nadir
    
    def print_over(self, to_print: str) -> None:
        print(to_print, end="\r")
    
    def confirmation(self, to_print: str = None) -> bool:
        """
        wait for the DM to confirm or decline with a button press
        Args:
            to_print (str): Guide text to print
        Returns:
            bool: true in confirmed, false if declined
        """
        if to_print is not None: print(to_print)
        while True:
            if self.buttons[0].click(): return True
            if self.buttons[1].click(): return False
    
    def choose_from(self, options: np.ndarray, index_start: int = 0) -> Tuple[int, object]:
        """
        Let the dm choose an option from a given list by scrolling through different options with buttons
        Args:
            options (str): The list of chooseable options
            index_start (int): The starting index of the search
        Raises:
            Exception: index_start is not a valid starting index. It is not between 0 and len(options) - 1
        Returns:
            (int, object): A tuple of the index of the option and the option from the array
        """
        if len(options) <= 0: 
            raise Exception("No options to choose from")

        index_max = len(options) - 1
        if (index_start < 0 or index_start > index_max):
            raise Exception("Starting index out of bounds")

        print('\nChoose a desired option: red +1, yellow -1, green confirm')
        current = index_start
        self.print_over(f"Currently chosen {current}/{index_max}: {options[current]}")
        while not self.buttons[0].click():
            if self.buttons[1].click():
                current = current +1 if current < index_max else 0
            elif self.buttons[2].click():
                current = current -1 if current > 0 else index_max
            else: continue # Print only if either of the buttons have been clicked

            self.print_over(f"Currently chosen {current}: {options[current]}")

        print()
        return current, options[current]
    
    # in desdeo_emo/docs/notebooks/Example.ipynb one can choose multiple preferred values. Also in nimbus (saving solutions)
    # Better implementation idea with the selection wheel:
        # Display all solutions at once, where?
        # highlight currently selected
        # Green to select, red to remove, mark selected
        # Hold the green button continue 
        # OR scroll to some continue button (which could be highlighted when selected count is valid) and press it
    def choose_multiple(self, options: np.ndarray, min_options: int = 1, max_options: int = None) -> List[List]:
        if max_options is None:
            max_options = len(options) # One can choose every option from the list
        if max_options > len(options):
            raise Exception("Can't choose more options than available")
        
        selected_options = []
        options_temp = options
        while (True):
            too_much = len(selected_options) >= max_options
            enough = len(selected_options) >= min_options
            
            if enough: # Don't ask for confirmation until min value reached
                if not self.confirmation("If you wish to add a new solution click the green button, if not red"):
                    break
            
            if too_much:
                print("Maximun options chosen")
                break
            
            # TODO Clean up
            option_temp = self.choose_from(options_temp) # This will mess up the indices
            options_temp = np.delete(options_temp, option_temp[0], 0) # Delete the option from the array so it won't be picked again
            option = (np.where(options == option_temp[1])[0][0], option_temp[1]) # Fix the index, TEMP
            selected_options.append(option)
            print(f"Current selection: {selected_options}")

        return selected_options
        

    
    def choose_from_range(self, index_min: float = 0, index_max: float = 100, index_start: float = None, step: float = 1) -> float:
        """
        Let the dm choose an number from a given range 
        Args:
            index_min (float): Smallest selectable value, defaults to 0
            index_max (float): Highest selectable value, defaults to 100
            index_start (float): The starting value, defaults to index_min
            step (float): the size of each step, defaults to 1
        Raises:
            Exception: index_min is not lower than index_max
            Exception: index_start is not a valid starting index. It is not between index_min and index_max
            Exception: step size is not valid
            Exception: starting index combined with step size is not valid
        Returns:
            float: the value chosen
        """
        if (index_min >= index_max):
            raise Exception("index_min is not lower than index_max")
        
        if (index_start is None):
            index_start = index_min

        if (index_start < index_min or index_start > index_max):
            raise Exception("Starting index out of bounds")
        
        if (step <= 0):
            raise Exception("Step size must a positive number greater than zero")

        if ((index_max - index_min) % step != 0):
            raise Exception("Step size is invalid, won't reach min or max values")
        
        if ((index_max - index_start) % step != 0):
            raise Exception("Won't reach min or max values with selected starting value and step size")

        print('\nChoose a desired option: red +1, yellow -1, green confirm')
        current = index_start
        self.print_over(f"Currently chosen {current}")

        while not self.buttons[0].click():
            if self.buttons[1].click():
                current = current +step if current < index_max else index_min
            elif self.buttons[2].click():
                current = current -step if current > index_min else index_max
            else: continue # Print only if either of the buttons have been clicked

            self.print_over(f"Currently chosen {current}")

        print()
        return current

    def get_potentiometer_value(self, value_name: str = "value", value_min: float = 0, value_max: float = 1, pot_index = 0) -> float:
        if pot_index >= len(self.potentiometers) or pot_index < 0:
            raise InterfaceException("invalid pot index")
        print("\nPress the green button when you're ready")
        while True:
            value = self.potentiometers[0].get_value(value_min, value_max)
            self.print_over(f"current {value_name}: {value}")
            if self.buttons[0].click(): break
        print("\n\n")
        return value
    
    def get_potentiometer_values(self, value_name: str = "values", value_min: float = 0, value_max: float = 1) -> np.array:
        """
        Display real time values from the pots to terminal and return the values when a button is pressed
        Args:
            value_name (str): Name of values. This will be printed to console and won't be necessary later on
            value_min (float): min bound for all values
            value_max (float): max bound for all values
        Returns:
            np.array: values chosen
        """
        print("\nPress the green button when you're ready")
        while True:
            values = list(map(lambda pot: pot.get_value(value_min, value_max), self.potentiometers))
            self.print_over(f"current {value_name}: {values}")
            if self.buttons[0].click(): break
        print("\n\n")
        return np.array(values)

    #I can merge both from below to the upper one but do i want to
    def get_potentiometer_bounded_values(self, value_name: str = "values") -> np.array:
        print("\nPress the green button when you're ready")
        if self.variable_bounds is None:
            return list(map(lambda pot: pot.get_value(), self.potentiometers))
        while True:
            values = []
            for pot in range(len(self.potentiometers)):
                bound_min = self.variable_bounds[pot][0]
                bound_max = self.variable_bounds[pot][1]
                values.append(self.potentiometers[pot].get_value(bound_min, bound_max))
            self.print_over(f"current {value_name}: {values}")
            if self.buttons[0].click(): break
        print()
        return np.array(values)
     
    def get_potentiometer_values_int(self, value_name: str = "values", value_min: int = 0, value_max: int = 1) -> np.array:
        print("\nPress the green button when you're ready")
        while True:
            values = list(map(lambda pot: pot.get_value_int(value_min, value_max), self.potentiometers))
            self.print_over(f"current {value_name}: {values}")
            if self.buttons[0].click(): break
        print("\n\n")
        return np.array(values)

    # TODO
    # Connect a component to the interface
    def component_connect(self, component: Component):
        return
    
    # TODO 
    # Check if the interface has all required components
    def validate_interface(self):
        return

# TODO mooooooore
if __name__ == "__main__":
    interface = Interface("COM3", button_pins=[2,3,4], potentiometer_pins = [0,1,2])

    t = np.array([1,2,3,4,5,6,7])
    print(f"Select at least 2 values but no more than 4 from this array: {t}")
    t_chosen = interface.choose_multiple(t, 2, 4)
    print(f"You chose these values: {t_chosen}")

    print("Choose a value between from 0-1500")
    v = interface.get_potentiometer_value(value_min=0, value_max=1500)
    

