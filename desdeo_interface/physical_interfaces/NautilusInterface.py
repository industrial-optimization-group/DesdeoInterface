import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.physical_interfaces.Interface import Interface
from desdeo_interface.components.Button import Button
from desdeo_interface.components.Potentiometer import Potentiometer
from time import sleep
import numpy as np
from typing import Union, Optional, List



class NautilusInterface(Interface):
    """
    A interface class for the Nautilus method
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.array, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.array, List[int]]): analog pins that are connected to potentiometers
        rotary_encoder_pins (Union[np.ndarray, List[List[int]]]): pairs of digital pins that are connected to rotary encoders
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    Raises:
        InterfaceException: Has less than three buttons.
    """

    it_count: int = 3 #Iteration count, default is 3
    preference: np.ndarray

    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]] = [],
        potentiometer_pins: Union[np.array, List[int]] = [],
        rotary_encoders_pins: Union[np.ndarray, List[List[int]]] = [],
        variable_bounds: Optional[np.ndarray] = None,
    ):
        super().__init__(port, button_pins, potentiometer_pins, rotary_encoders_pins, variable_bounds)
    
    def get_iteration_count(self) -> int:
        print("\nSet a new iteration count")
        return int(self.choose_from_range(index_min = 1, index_start = 3))
    
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
    
    # This would be much easier and cleaner with rotary encoder
    # Also I'd like that each value could be adjusted at the same time, now we go one by one
    def get_preference_info_percentages(self):
        print("\nUse the potentiometers to set percentages to the objectives, green to confirm")
        n = len(self.potentiometers) # I'd rather get n from somewhere else
        percentages = []
        used_percentage = 0
        for pot in range(n-1):
            percentage = self.get_potentiometer_value(f"pot {pot}: percentage", 0, 100-used_percentage, pot)
            used_percentage += percentage
            percentages.append(percentage)
        percentages.append(100-used_percentage) # Last one depends on the other selections
        print(f"Chosen percentages: {percentages}")
        return percentages


#Testing
if __name__ == "__main__":
    from desdeo_problem.Variable import variable_builder
    from desdeo_problem.Objective import VectorObjective, _ScalarObjective
    from desdeo_problem.Problem import MOProblem

    from desdeo_mcdm.interactive import Nautilus
    # example problem from article

    def f1(xs):
        xs = np.atleast_2d(xs)
        return -4.07 - 2.27 * xs[:, 0]


    def f2(xs):
        xs = np.atleast_2d(xs)
        return -2.60 - 0.03 * xs[:, 0] - 0.02 * xs[:, 1] - (0.01 / (1.39 - xs[:, 0] ** 2)) - (
                0.30 / (1.39 - xs[:, 1] ** 2))


    def f3(xs):
        xs = np.atleast_2d(xs)
        return -8.21 + (0.71 / (1.09 - xs[:, 0] ** 2))


    def f4(xs):
        xs = np.atleast_2d(xs)
        return -0.96 + (0.96 / (1.09 - xs[:, 1] ** 2))


    def objectives(xs):
        return np.stack((f1(xs), f2(xs), f3(xs))).T


    obj1 = _ScalarObjective("obj1", f1)
    obj2 = _ScalarObjective("obj2", f2)
    obj3 = _ScalarObjective("obj3", f3)

    objkaikki = VectorObjective("obj", objectives)

    # variables
    var_names = ["x1", "x2"]  # Make sure that the variable names are meaningful to you.

    initial_values = np.array([0.5, 0.5])
    lower_bounds = [0.3, 0.3]
    upper_bounds = [1.0, 1.0]
    bounds = np.stack((lower_bounds, upper_bounds))
    variables = variable_builder(var_names, initial_values, lower_bounds, upper_bounds)

    # problem
    prob = MOProblem(objectives=[obj1, obj2, obj3], variables=variables)  # objectives "seperately"

    ideal = np.array([-6.34, -3.44487179, -7.5])
    nadir = np.array([-4.751, -2.86054116, -0.32111111])
    print("Ideal: ", ideal)
    print("Nadir: ", nadir)


    # Interface
    interface = NautilusInterface("COM3", button_pins = [2,3,4], potentiometer_pins= [0,1,2], variable_bounds= np.column_stack((ideal, nadir)))

    # start solving
    method = Nautilus(problem=prob, ideal=ideal, nadir=nadir)

    print("Let's start solving\n")
    req = method.start()

    # initial preferences

    it_count = interface.get_iteration_count()

    preference_method = interface.get_preference_method()

    if preference_method == 1:
        preference_info = interface.get_preference_info_relative_ranking()
    else: 
        preference_info = interface.get_preference_info_percentages()
    
    req.response = {
        "n_iterations": it_count,
        "preference_method": preference_method,
        "preference_info": np.array(preference_info),
    }
    print("Step number: 0")
    print("Iteration point: ", nadir)
    print("Lower bounds of objectives: ", ideal)

    req = method.iterate(req)
    print("\nStep number: ", method._step_number)
    print("Iteration point: ", req.content["current_iteration_point"])
    print("Pareto optimal vector: ", method._fs[method._step_number])
    print("Lower bounds of objectives: ", req.content["lower_bounds"])
    print("Upper bounds of objectives:", req.content["upper_bounds"])
    print("Closeness to Pareto optimal front", req.content["distance"])


    while (method._step_number < it_count):

        step_back = interface.step_back() 
        short_step = interface.short_step() if step_back else False

        use_previous_preference = False if step_back else interface.use_previous_preference()
        if not use_previous_preference:
            interface.get_preference_method()

            if preference_method == 1:
                preference_info = interface.get_preference_info_relative_ranking()
            else: 
                preference_info = interface.get_preference_info_percentages()

        req.response = {
            "step_back": step_back,
            "short_step": short_step,
            "use_previous_preference": use_previous_preference,
            "preference_method": preference_method,
            "preference_info": np.array(preference_info),
        }

        req = method.iterate(req)

        print("\nStep number: ", method._step_number)
        print("Iteration point: ", req.content["current_iteration_point"])
        print("Pareto optimal vector: ", method._fs[method._step_number])
        print("Lower bounds of objectives: ", req.content["lower_bounds"])
        print("Upper bounds of objectives:", req.content["upper_bounds"])
        print("Closeness to Pareto optimal front", req.content["distance"])