from components import Button, Potentiometer
from pyfirmata import Arduino, util
from time import sleep
import numpy as np
from typing import Union, Optional, List, Tuple


class InterfaceException(Exception):
    """
    Raised when an exception related to an interface is encountered.
    """

    pass


class Interface:
    """
    A interface class to handle the Decision Maker's inputs given with a physical interface (Arduino)
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.array, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.array, List[int]]): analog pins that are connected to potentiometers, count should be the same as variables
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    Raises:
        InterfaceException: more variables than potentiometers, cant adjust each variable
        InterfaceException: Has less than two buttons.
        SerialException from pyfirmata: Can't open serialport. Already in use or not plugged
    """

    _board: Arduino
    _it: util.Iterator
    buttons: list[Button]
    potentiometers: list[Potentiometer]
    variable_bounds: Optional[np.ndarray]

    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]],
        potentiometer_pins: Union[np.array, List[int]],
        variable_bounds: Optional[np.ndarray],
    ):
        if variable_bounds is not None:
            if len(variable_bounds) > len(potentiometer_pins):
                raise InterfaceException(
                    "Not enough potentiometers!"
                )
        if len(button_pins) < 3: #One could use only one button, if the button has multiple interactions, hold, double click and so on. Two is easier and three very simple
            raise InterfaceException("A physical interface requires atleast three buttons")

        self._board = Arduino(port)
        self._it = util.Iterator(self._board)
        self._it.start()

        self.buttons = list(map(lambda pin: Button(self._board, pin), button_pins))
        self.potentiometers = list(
            map(lambda pin: Potentiometer(self._board, pin), potentiometer_pins)
        )
        if variable_bounds is not None:
            self.variable_bounds = variable_bounds
            self.potentiometers = self.potentiometers[:len(variable_bounds)] #Cut off unneeded potentiometers
        else:
            k = len(potentiometer_pins)
            self.variable_bounds = np.column_stack((np.zeros(k), np.ones(k)))
    
    def print_over(self, to_print: str) -> None:
        print(to_print, end="\r")
    
    def confirmation(self, to_print: str = "") -> bool:
        """
        wait for the DM to confirm or decline with a button press
        Args:
            to_print (str): Guide text to print

        Returns:
            bool: true in confirmed, false if declined
        """
        if to_print: print(to_print)
        while True:
            if self.buttons[0].click(): return True
            if self.buttons[1].click(): return False
    
    #fix, if has options then max should prob be the count of elements - 1
    def choose_from(self, options: np.ndarray = None, index_min = 0, index_max = 100, index_start = 0): # -> ??? element of the array or the index, could be many things
        print('\nNavigate through different options: red +1, yellow -1, green confirm')
        current = index_start
        if options is None: 
            self.print_over(f"Currently chosen {current}")
        else:
            self.print_over(f"Currently chosen {current}: {options[current]}")
        while not self.buttons[0].click():
            if self.buttons[1].click():
                current = current +1 if current < index_max else index_min
            elif self.buttons[2].click():
                current = current -1 if current > index_min else index_max
            else: continue

            if options is None:
                self.print_over(f"Currently chosen {current}")
            else:
                self.print_over(f"Currently chosen {current}: {options[current]}")
        print()
        return current if options is None else (current, options[current])

    def get_potentiometer_value(self, value_name: str = "value", value_min: float = 0, value_max: float = 1, pot_index = 0) -> float:
        if pot_index  >= len(self.potentiometers):
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
            value_name (str): Name of values. This will be printed to console and won't be neseccary later on
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
        



class NautilusInterface(Interface):
    """
    A interface class for the Nautilus method
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.array, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.array, List[int]]): analog pins that are connected to potentiometers
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    Raises:
        InterfaceException: Has less than three buttons.
    """

    it_count: int = 3 #Iteration count, default is 3
    preference: np.ndarray

    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]],
        potentiometer_pins: Union[np.array, List[int]],
        variable_bounds: Optional[np.ndarray],
    ):
        super().__init__(port, button_pins, potentiometer_pins, variable_bounds)

        if len(button_pins) < 3:
            raise Exception("Nautilus interface requires atleast three buttons")
    
    def get_iteration_count(self) -> int:
        print("\nSet a new iteration count")
        return self.choose_from(index_min = 1, index_start = 3)
    
    def step_back(self) -> bool:
        return self.confirmation("\nDo you wish to step back? green yes, red no")
    
    def short_step(self) -> bool:
        return self.confirmation("\nshort step? green yes, red no")
    
    def use_previous_preference(self):
        return self.confirmation("\nDo you wish to use previous preference? green yes, red no")
        
    def get_preference_method(self): #Maybe to parent as select from
        print("\nSelect the method: green = relative_ranking, red = percentages")
        while True:
            if self.buttons[0].click(): return 1
            if self.buttons[1].click(): return 2
    
    def get_preference_info_relative_ranking(self):
        print("\nUse the potentiometers to set relative rankings to the objectives, green to confirm")
        bound_max = len(self.potentiometers)
        return self.get_potentiometer_values_int("rankings", 1, bound_max)
    
    #fix
    def get_preference_info_percentages(self):
        print("\nUse the potentiometers to set percentages to the objectives, green to confirm")
        used_percentage = 0
        while not self.buttons[0].click():
            for pot in self.potentiometers:
                percentage = pot.get_value(0,100-used_percentage)
                used_percentage += percentage
                percentages = []
                percentages.append(percentage)
            self.print_over(f"Current percentages: {percentages}")
        print("\n\n")
        return percentages




class RPMInterface(Interface):
    """
    A interface class for the reference point method
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.array, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.array, List[int]]): analog pins that are connected to potentiometers
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    """

    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]],
        potentiometer_pins: Union[np.array, List[int]],
        variable_bounds: Optional[np.ndarray],
    ):
        super().__init__(port, button_pins, potentiometer_pins, variable_bounds)


    def get_input(self) -> np.array:
        """
        Get real time values from potentiometers and display them. Continue when the DM chooses to

        Returns:
            np.array: new reference point
        """
        return self.get_potentiometer_bounded_values("Reference point")

    def get_satisfaction(self) -> bool:
        return self.confirmation("\npress the green button if satisfied, else red")

    # fix, double clicks and stuff
    def pick_solution(self, solutions: np.ndarray) -> int:
        """
        Let the DM choose the desired solution from the different options

        Args:
            solutions [np.array]: pareto optimal solution and additional solutions

        Returns:
            int: Index of the desired solution
        """
        print(
            """Choose a desired solution by scrolling through the options by clicking the red button
        when ready press the green button"""
        )

        return self.choose_from(solutions, index_max = len(solutions) - 1)[0]



#So bad, wip
class NimbusInterface(Interface):
    """
    A interface class for the Nimbus method
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.array, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.array, List[int]]): analog pins that are connected to potentiometers
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    """

    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]],
        potentiometer_pins: Union[np.array, List[int]],
        variable_bounds: Optional[np.ndarray],
    ):
        super().__init__(port, button_pins, potentiometer_pins, variable_bounds)
    
    #fix, bounds
    def get_aspiration_level(self) -> float:
        print(f"Specify an aspiration level for the objective")
        return self.get_potentiometer_value()
    
    #fix, bounds
    def get_upper_bound(self) -> float:
        print(f"Specify a upper bound for the objective")
        return self.get_potentiometer_value()
    
    def get_classification(self, options: Union[List[str], np.ndarray]) -> str:
        return self.choose_from(options, index_max=len(options) - 1)[1]
    
    def get_classifications_and_levels(self, amount) -> Tuple[List[str], List[float]]:
        classification_options = ["<", "<=", "=", ">=", "0"]
        classifications = []
        levels = []
        for obj_value in range(amount):
            print(f"Pick a classification level for objective at index {obj_value}")
            classification = self.get_classification(classification_options)
            if classification == "<=":
                level = self.get_aspiration_level()
            elif classification == ">=":
                level = self.get_upper_bound()
            else:
                level = -1
            levels.append(level)
            classifications.append(classification)
            print(f"Current classifications and levels: {classifications} {levels}")
        return classifications, levels
    
    def specify_solution_count(self):
        return 0
    
    def pick_preferred_solution(self):
        return 0
    
    def should_continue(self):
        return 0

    def try_another_classification(self):
        return 0
    
    def show_different_alternatives(self):
        return 0