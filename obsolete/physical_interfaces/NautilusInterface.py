import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_problem.Problem import MOProblem
from desdeo_interface.components.Master import Master
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

    def __init__(
        self,
        problem: MOProblem,
        button_pins: Union[np.array, List[int]] = [],
        potentiometer_pins: Union[np.array, List[int]] = [],
        rotary_encoders_pins: Union[np.ndarray, List[List[int]]] = [],
    ):
        super().__init__(problem, True)
    
    def get_iteration_count(self) -> int:
        """
        Choose the iteration count for nautilusnavigator

        Args:
            index_start (int): The starting point

        Returns:
            int: Desired iteration count
        """
        print("\nSet a new iteration count")
        return int(self.choose_value(index_min = 1))
    
    def step_back(self) -> bool:
        """
        Does the DM want to step back

        Returns:
            bool: Whether or not to step back 
        """
        return self.confirmation("\nDo you wish to step back?")
    
    def short_step(self) -> bool:
        """
        Does the DM want to take short step when stepping back

        Returns:
            bool: Whether or not to short step 
        """
        return self.confirmation("\nshort step?")
    
    def use_previous_preference(self):
        """
        Does the DM want to use the previous preference

        Returns:
            bool: Whether or not to use previous preference
        """
        return self.confirmation("\nDo you wish to use previous preference?")
        
    def get_preference_method(self):
        """
        Ask the DM for the desired preference method

        Returns:
            int: The desired preference method, where 1 is relative ranking and 2 is percentages
        """
        print("\nSelect the method")
        options = np.array(["Relative", "Percentages"])
        return self.choose_from(options)[0] + 1
    
    def get_preference_info_relative_ranking(self):
        """
        Get relative rankings for objective preferences

        Returns:
            List[int]: A list of relative rankings for each objective
        """
        print("\nUse the variable handlers to set relative rankings to the objectives, green to confirm")
        objective_count = self.problem.n_of_objectives
        lower_bounds = np.array([1]*objective_count)
        upper_bounds = np.array([objective_count]*objective_count)
        bounds = np.stack((lower_bounds, upper_bounds)) 
        return self.get_values(bounds, 1, True)
    
    def get_preference_info_percentages(self):
        """
        Get percentages for objective preferences

        Returns:
            List[int]: A list of percentages for each objective
        """
        print("\nUse the variable handlers to set percentages to the objectives")
        objective_count = self.problem.n_of_objectives
        lower_bounds = np.array([0]*objective_count)
        upper_bounds = np.array([100]*objective_count)
        bounds = np.stack((lower_bounds, upper_bounds))
        values = self.get_values(bounds, 1, True)
        values_sum = sum(values)
        if values_sum > 100:
            part = int(np.floor((values_sum - 100)/objective_count))
            values = [(value - part) for value in values]
        elif values_sum < 100:
            part = int(np.ceil((100 - values_sum)/objective_count))
            values = [(value + part) for value in values]
        overflow = sum(values) - 100
        for i in range(overflow):
            values[i] = values[i] - 1
        return values


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

    ideal = np.array([-6.34, -3.44487179, -7.5])
    nadir = np.array([-4.751, -2.86054116, -0.32111111])
    print("Ideal: ", ideal)
    print("Nadir: ", nadir)
    # problem
    prob = MOProblem(objectives=[obj1, obj2, obj3], variables=variables, ideal=ideal, nadir=nadir)  # objectives "seperately"

    # Interface
    master = Master()
    interface = NautilusInterface(prob)

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
            preference_method = interface.get_preference_method()
            
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