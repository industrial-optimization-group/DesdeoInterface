import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)
from desdeo_interface.components.Master import Master
from desdeo_interface.components.Button import Button
from desdeo_interface.components.Potentiometer import Potentiometer
from desdeo_interface.components.RotaryEncoder import RotaryEncoder
from desdeo_interface.components.Component import Component
from desdeo_interface.components.SerialReader import SerialReader

from desdeo_problem.Variable import Variable
from desdeo_problem.Problem import MOProblem
import threading
from time import sleep
import time
import numpy as np
from typing import Union, Optional, List, Tuple, Any

class InterfaceException(Exception):
    """
    Raised when an exception related to an interface is encountered.
    """

    pass

class Interface:
    """
    A interface class to handle the Decision Maker's inputs given with a physical interface (Arduino)
    Args:
        button_pins (Union[np.ndarray, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.ndarray, List[int]]): analog pins that are connected to potentiometers, count should be the same as variables
        rotary_encoder_pins (Union[np.ndarray, List[List[int]]]): pairs of digital pins that are connected to rotary encoders
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    Raises:
        InterfaceException: more variables than potentiometers + rotary_encoders, cant adjust each variable
        InterfaceException: Has less than two buttons.
    """

    master: Master
    buttons: List[Button]
    problem: MOProblem
    value_handlers: List[Union[Potentiometer, RotaryEncoder]]

    #Todo default values
    def __init__(
        self,
        problem: MOProblem,
        handle_objectives: bool = True
    ):
        self.master = Master()
        self.serial_reader = SerialReader()
        self.problem = problem
        self.targets = {}

        # Updates the 'raw' data which is read from the serial
        # self.raw_updater = threading.Thread(target=self.serial_reader.update, daemon=True)
        # self.raw_updater.start()
        self.construct(handle_objectives)

        # Updates each component
        self.updater = threading.Thread(target=self.update, daemon=True)
        self.updater.start()

        # self.value_handlers = [
        #     target['component'] for target
        #     in list(self.targets.values()) 
        #     if target["component_info"][0] == "R"
        #     or target["component_info"][0] == "P"
        # ]

        # print(self.value_handlers)
    
    # If to_print doesn't fit on one line then its just gonna print to a lot of lines
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
        return self.master.confirm()

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

        index_max = len(options)
        if (index_start < 0 or index_start > index_max):
            raise Exception("Starting index out of bounds")

        self.master.wheel.current_value = index_start

        while not self.master.confirm_button.click():
            current = self.master.select(0, index_max)
            self.print_over(f"Currently chosen {current}/{index_max - 1}: {options[current]}")

        print()
        return current, options[current]
    
    # in desdeo_emo/docs/notebooks/Example.ipynb one can choose multiple preferred values. Also in nimbus (saving solutions)
    def choose_multiple(self, options: np.ndarray, min_options: int = 1, max_options: int = None) -> List[List]:
        if max_options is None:
            max_options = len(options) # choose all options from the list if wished so
        if max_options > len(options):
            raise Exception("Can't choose more options than available")
        
        selected_options = []
        options_temp = options
        while (True):
            too_much = len(selected_options) >= max_options
            enough = len(selected_options) >= min_options

            if too_much:
                print("Maximun options chosen")
                break

            if enough: # Don't ask for confirmation until min value reached
                if not self.confirmation("If you wish to add a new solution click the confirm button, if not decline"):
                    break
            
            # TODO Clean up
            option_temp = self.choose_from(options_temp) # This will mess up the indices
            options_temp = np.delete(options_temp, option_temp[0], 0) # Delete the option from the array so it won't be picked again
            option = (np.where(options == option_temp[1])[0][0], option_temp[1]) # Fix the index, TEMP
            selected_options.append(option)
            print(f"Current selection: {selected_options}")

        return selected_options
        
    
    def choose_value(self, index_min: float = 0, index_max: float = 100, index_start: float = None, step: float = 1) -> float:
        """
        Let the dm choose an number from a given range 
        Args:
            index_min (float): Smallest selectable value, defaults to 0
            index_max (float): Highest selectable value, defaults to 100
            index_start (float): The starting value, defaults to index_min
            step (float): the size of each step, defaults to 1
        Raises:
            Exception: Some incompatibility with the given indexes and step size
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

        self.master.wheel.current_value = index_start

        while not self.master.confirm_button.click():
            current = self.master.select(index_min, index_max)
            self.print_over(f"Currently chosen {current}")
        print()

        return current

    # def get_potentiometer_value(self, value_name: str = "value", value_min: float = 0, value_max: float = 1, pot_index = 0) -> float:
    #     if pot_index >= len(self.potentiometers) or pot_index < 0:
    #         raise InterfaceException("invalid pot index")
    #     print("\nPress the green button when you're ready")
    #     while True:
    #         value = self.potentiometers[0].get_value(value_min, value_max)
    #         self.print_over(f"current {value_name}: {value}")
    #         if self.buttons[0].click(): break
    #     print("\n\n")
    #     return value
    
    def get_variable_value(self, variable):
        bound_min, bound_max = variable.get_bounds()
        index = self.problem.variables.index(variable)
        return self.value_handlers[index].get_value(bound_min, bound_max)
    
    def get_variable_values(self):
        while True:
            if self.master.confirm_button.click(): break
            values = list(map(lambda var: self.get_variable_value(var), self.problem.variables))
            self.print_over(values)
        return values

    # def get_value_old(self, index: int, bounds: np.ndarray):
    #     bound_min, bound_max = bounds
    #     return self.value_handlers[index].get_value(bound_min, bound_max)
    
    def get_values(self, bounds: np.ndarray, int_values = False):
        while True:
            if self.master.confirm_button.click(): break
            values = []
            for target in self.targets:
                i = list(self.targets).index(target)
                value = self.get_value(target, bounds[:,i], int_values)
                values.append(value)
            self.print_over(values)
        return values
    
    def construct(self, handle_objectives):
        print("Constructing the interface...")
        time.sleep(3) # Let the thread get values
        self.serial_reader.update()
        raw_data = self.serial_reader.data()
        print(f"found {len(raw_data.keys()) - 1} modules")
        
    
        master_data = raw_data.pop('master')
        self.update_master(master_data)

        values_to_handle = self.problem.objectives if handle_objectives else self.problem.variables

        p_count = sum([len(node['P']) for node in raw_data.values() if 'P' in node])
        r_count = sum([len(node['R']) for node in raw_data.values() if 'R' in node])

        print(f"found {p_count + r_count} components in total")

        if len(values_to_handle) > p_count + r_count:
            raise Exception("Not enough handlers")

        for node_id in raw_data.keys():
            for component_type in raw_data[node_id].keys():
                for component_id in raw_data[node_id][component_type].keys():
                    if len(values_to_handle) == 0: break # All variables or objectives are assigned
                    self.assign_component(node_id, component_type, component_id, values_to_handle)
        print("successfully constructed the interface")
    
    def assign_component(self, node_id, component_type, component_id, values_to_handle):
        if component_type == "B":
            print("Skipping button")
            return
        next = values_to_handle.pop() # What about vector objectives
        if component_type == "P":
            print(f"Adding a potentiometer to {next.name}")
            comp = Potentiometer()
        elif component_type == "R": 
            print(f"Adding a rotary encoder to handle {next.name}")
            comp = RotaryEncoder()
        else: return
        if isinstance(next, Variable):
             lower, upper = next.get_bounds()
        else:
             lower = next.lower_bound
             upper = next.upper_bound
        self.targets[next.name] = {
            'component': comp,
            'component_info': (component_type, component_id),
            'node': node_id,
            'value': 0,
            'lower_bound': lower,
            'upper_bound': upper,
        }
    
    def get_value(self, target_name, bounds, int_values = False):
        bound_min, bound_max = bounds
        value = self.targets[target_name]['component'].get_value(bound_min, bound_max)
        if int_values: value =  np.round(value)
        return value
        
    def update(self):
        while True:
            self.serial_reader.update()
            if self.targets is None: return 
            raw_data = self.serial_reader.data()
            if raw_data is None: return
            master_data = raw_data.pop("master")
            self.update_master(master_data)
            for target in self.targets.values():
                node_id = target['node']
                component_type, component_id = target['component_info']
                value = raw_data[node_id][component_type][component_id]
                component = target["component"]
                # if isinstance(value, int): value = [value]
                component.update(value)
                target["value"] = value

    # MAYBE remove master
    def update_master(self, data):
        self.master.confirm_button.update(data['Accept'])
        self.master.decline_button.update(data['Decline'])
        self.master.wheel.update(data['Rotary'])

    # TODO 
    # Check if the interface has all required components
    def validate_interface(self):
        return

# TODO testing
if __name__ == "__main__":
    pass
    # master = Master("COM3", 3,2,[8,9])
    # interface = Interface(master, variables=[],potentiometer_pins = [0,1,2])
    # t = np.array([1,2,3,4,5,6,7])
    # print(f"Select at least 2 values but no more than 4 from this array: {t}")
    # t_chosen = interface.choose_multiple(t, 2, 4)
    # print(f"You chose these values: {t_chosen}")
    

