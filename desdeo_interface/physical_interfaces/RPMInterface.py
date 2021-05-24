import os, sys
from desdeo_mcdm.interactive.ReferencePointMethod import RPMException
from desdeo_problem.Variable import Variable
from desdeo_problem.Problem import MOProblem

from numpy.core.fromnumeric import var
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.physical_interfaces.Interface import Interface
from desdeo_interface.components.Button import Button
from desdeo_interface.components.Potentiometer import Potentiometer
from desdeo_interface.components.Master import Master
from time import sleep
import numpy as np
from typing import Union, Optional, List

class RPMInterface(Interface):
    """
    A interface class for the reference point method
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.ndarray, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.ndarray, List[int]]): analog pins that are connected to potentiometers
        rotary_encoder_pins (Union[np.ndarray, List[List[int]]]): pairs of digital pins that are connected to rotary encoders
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    """

    def __init__(
        self,
        master: Master,
        problem: MOProblem,
        button_pins: Union[np.array, List[int]] = [],
        potentiometer_pins: Union[np.array, List[int]] = [],
        rotary_encoders_pins: Union[np.ndarray, List[List[int]]] = [],
    ):
        super().__init__(master, problem, button_pins, potentiometer_pins, rotary_encoders_pins)
        if len(problem.objectives) > len(self.value_handlers):
            raise RPMException("Not enough variable handlers")


    def get_referencepoint(self) -> np.ndarray:
        """
        Get a new referencepoint from the dm
        Returns:
            np.array: new reference point
        """ 
        return np.array(self.get_values(np.stack((self.problem.ideal, self.problem.nadir))))

    def get_satisfaction(self) -> bool:
        """
        Get the DM's satisfaction with the current solutions

        Returns:
            bool: whether or not the DM is satisfied
        """
        return self.confirmation("\npress the green button if satisfied, else red")

    def pick_solution(self, solutions: np.ndarray) -> int:
        """
        Let the DM choose the desired solution from the different options

        Args:
            solutions [np.array]: pareto optimal solution and additional solutions

        Returns:
            int: Index of the desired solution
        """
        print("Choose a desired solution")
        return self.choose_from(solutions)[0]


# testing the method
if __name__ == "__main__":
    from desdeo_mcdm.interactive.InteractiveMethod import InteractiveMethod
    from desdeo_problem.Objective import VectorObjective, _ScalarObjective
    from desdeo_problem.Problem import MOProblem
    from desdeo_problem.Variable import variable_builder

    from desdeo_mcdm.interactive import ReferencePointMethod
    print("Reference point method")

    # Objectives
    def f1(xs):
        xs = np.atleast_2d(xs)
        return -4.07 - 2.27 * xs[:, 0]

    def f2(xs):
        xs = np.atleast_2d(xs)
        return (
            -2.60
            - 0.03 * xs[:, 0]
            - 0.02 * xs[:, 1]
            - (0.01 / (1.39 - xs[:, 0] ** 2))
            - (0.30 / (1.39 - xs[:, 1] ** 2))
        )

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
    obj4 = _ScalarObjective("obj4", f4)

    objkaikki = VectorObjective("obj", objectives)


    # variables
    var_names = ["x1", "x2"]  # Make sure that the variable names are meaningful to you.

    initial_values = np.array([0.5, 0.5])
    lower_bounds = [0.3, 0.3]
    upper_bounds = [1.0, 1.0]
    bounds = np.stack((lower_bounds, upper_bounds))
    variables = variable_builder(var_names, initial_values, lower_bounds, upper_bounds)

    # solved in Nautilus.py
    ideal = np.array([-6.34, -3.44487179, -7.5])
    nadir = np.array([-4.751, -2.86054116, -0.32111111])

    # problem
    prob = MOProblem(objectives=[obj1, obj2, obj3], variables=variables, ideal=ideal, nadir=nadir)  # objectives "seperately"


    # interface
    master = Master("COM3", 3, 2 ,[8,9])
    interface = RPMInterface(master, prob, potentiometer_pins= [0,1,2])

    # start solving
    method = ReferencePointMethod(problem=prob, ideal=ideal, nadir=nadir)

    # Pareto optimal solution: [-6.30, -3.26, -2.60, 3.63]

    print("Let's start solving\n")
    print(f"nadir: {nadir}")
    print(f"ideal: {ideal}\n")
    
    req = method.start()
    rp = interface.get_referencepoint()
    req.response = {
        "reference_point": rp,
    }

    print(f"Step number: {method._h} \n")

    req = method.iterate(req)
    step = 1
    print("\nStep number: ", method._h)
    print("Reference point: ", rp)
    print("Pareto optimal solution: ", req.content["current_solution"])
    print("Additional solutions: ", req.content["additional_solutions"])

    satisfied = interface.get_satisfaction()
    while not satisfied:
        step += 1
        rp = rp = interface.get_referencepoint()
        req.response = {"reference_point": rp, "satisfied": False}
        req = method.iterate(req)
        print("\nStep number: ", method._h)
        print("Reference point: ", rp)
        print("Pareto optimal solution: ", req.content["current_solution"])
        print("Additional solutions: ", req.content["additional_solutions"])
        satisfied = interface.get_satisfaction()
    
    
    
    solutions = np.concatenate(([req.content["current_solution"]], req.content["additional_solutions"]))
    solution_index = interface.pick_solution(solutions)
    req.response = {"satisfied": satisfied, "solution_index": solution_index}
    req = method.iterate(req)

    print(req.content)