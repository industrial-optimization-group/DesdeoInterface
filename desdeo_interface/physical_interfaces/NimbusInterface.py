import os, sys

from numpy.core.fromnumeric import var
from numpy.core.numeric import indices
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.physical_interfaces.Interface import Interface
from desdeo_interface.components.Button import Button
from desdeo_interface.components.Potentiometer import Potentiometer
from time import sleep
import numpy as np
from typing import Union, Optional, List, Tuple

class NimbusInterface(Interface):
    """
    A interface class for the Nimbus method
    Args:
        port (str): The serial port Arduino is connected
        button_pins (Union[np.array, List[int]]): digital pins that are connected to buttons
        potentiometer_pins (Union[np.array, List[int]]): analog pins that are connected to potentiometers
        rotary_encoder_pins (Union[np.ndarray, List[List[int]]]): pairs of digital pins that are connected to rotary encoders
        variable_bounds (Optional[np.ndarray]): Bounds for reference points, defaults to [0,1] for each variable
    """

    def __init__(
        self,
        port: str,
        button_pins: Union[np.array, List[int]] = [],
        potentiometer_pins: Union[np.array, List[int]] = [],
        rotary_encoders_pins: Union[np.ndarray, List[List[int]]] = [],
        variable_bounds: Optional[np.ndarray] = None,
    ):
        super().__init__(port, button_pins, potentiometer_pins, rotary_encoders_pins, variable_bounds)
    
    def get_aspiration_level(self, min, max) -> float:
        """
        Get an aspiration level for an objective

        Args:
            min (float): Minimum value for the aspiration level
            max (float): Maximum value for the aspiration level

        Returns:
            float: Chosen aspiration level
        """
        print(f"Specify an aspiration level for the objective")
        return self.get_potentiometer_value(value_name="Aspiration level", value_min= min, value_max= max)

    def get_upper_bound(self, min, max) -> float:
        """
        Get an upper bound for an objective

        Args:
            min (float): Minimum value for the bound
            max (float): Maximum value for the bound

        Returns:
            float: Chosen upper bound
        """
        print(f"Specify a upper bound for the objective")
        return self.get_potentiometer_value(value_name="Upper bound", value_min= min, value_max= max)
    
    def get_classification(self) -> str:
        """
        Choose a classification for an objective from ["<", "<=", "=", ">=", "0"]

        Returns:
            str: Chosen classification
        """
        classification_options = ["<", "<=", "=", ">=", "0"]
        return self.choose_from(classification_options)[1]
    
    def get_classifications_and_levels(self, objective_count) -> Tuple[List[str], List[float]]:
        """
        Choose a classification and levels for each objective 

        Args:
            objective_count (int): how many objectives to classificate

        Returns:
            str: Chosen classification
        """
        classifications = []
        levels = []
        for obj_index in range(objective_count):
            print(f"Pick a classification level for objective at index {obj_index}")
            classification = self.get_classification()
            if classification == "<=":
                min, max = self.variable_bounds[obj_index]
                level = self.get_aspiration_level(min, max)
            elif classification == ">=":
                min, max = self.variable_bounds[obj_index]
                level = self.get_upper_bound(min, max)
            else:
                level = -1
            levels.append(level)
            classifications.append(classification)
            print(f"Current classifications and levels: {classifications} {levels}")
        return classifications, levels
    
    def specify_solution_count(self):
        """
        Specify the amount of solutions to be calculated in the next step

        Returns:
            int: The amount of solutions to be calculated
        """
        print("Select solution count")
        return self.choose_from_range(1, 4)
    
    def pick_preferred_solution(self, solutions: np.ndarray):
        """
        Let the DM pick a preferred solution from a list of solutions

        Returns:
            Any: The preferred solution
        """
        return self.choose_from(solutions)[0]
    
    def should_continue(self):
        """
        Should the method be continued or stopped

        Returns:
            bool: Whether or not to continue
        """
        return self.confirmation("Continue, green yes, red no")

    def try_another_classification(self):
        """
        Does the DM want to try another set of classifications

        Returns:
            bool: Whether or not to change classifications
        """
        return self.confirmation("Try another classification, green yes, red no")
    
    def show_different_alternatives(self):
        """
        Does the DM want to see different alternatives

        Returns:
            bool: Whether or not to see different alternatives
        """
        return self.confirmation("Show alternatives, green yes, red no")
    
    def save_solutions(self, solutions):
        """
        Let the DM pick solutions they want to be saved

        Args:
            solutions (np.ndarray): Array of selectable solutions

        Returns:
            List: A list of the solutions the DM wants to save
        """
        selected_solutions = self.choose_multiple(solutions, 0) # Get the solutions
        selected_solutions = list(map(lambda s: s[0], selected_solutions)) # Only get the indices
        return selected_solutions

    def choose_two_solutions(self, solutions):
        """
        Let the DM choose two solutions for the intermediate solutions step

        Args:
            solutions (np.ndarray): Array of selectable solutions

        Returns:
            List: A list with the two chosen solutions
        """
        selected_solutions = self.choose_multiple(solutions,2,2)
        selected_solutions = list(map(lambda s: s[0], selected_solutions))
        return selected_solutions


if __name__ == "__main__":
    import matplotlib.pyplot as plt
    from desdeo_problem.Problem import MOProblem
    from desdeo_problem.Variable import variable_builder
    from desdeo_problem.Objective import _ScalarObjective

    def plot(request_type: str):
        plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
        plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
        plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
        if request_type == "preferred":
            for i, z in enumerate(preferred_request.content["objectives"]):
                plt.scatter(z[0], z[1], label=f"solution {i}")
        elif request_type == "save":
            for i, z in enumerate(save_request.content["objectives"]):
                plt.scatter(z[0], z[1], label=f"solution {i}")
        else:
            for i, z in enumerate(intermediate_request.content["objectives"]):
                plt.scatter(z[0], z[1], label=f"solution {i}")
        plt.xlabel("f1")
        plt.ylabel("f2")
        plt.title("Approximate Pareto front of the Kursawe function")
        plt.legend()
        plt.show()
    
    def f_1(xs: np.ndarray):
        xs = np.atleast_2d(xs)
        xs_plusone = np.roll(xs, 1, axis=1)
        return np.sum(-10*np.exp(-0.2*np.sqrt(xs[:, :-1]**2 + xs_plusone[:, :-1]**2)), axis=1)

    def f_2(xs: np.ndarray):
        xs = np.atleast_2d(xs)
        return np.sum(np.abs(xs)**0.8 + 5*np.sin(xs**3), axis=1)


    varsl = variable_builder(
        ["x_1", "x_2", "x_3"],
        initial_values=[0, 0, 0],
        lower_bounds=[-5, -5, -5],
        upper_bounds=[5, 5, 5],
    )

    f1 = _ScalarObjective(name="f1", evaluator=f_1)
    f2 = _ScalarObjective(name="f2", evaluator=f_2)

    nadir=np.array([-14, 0.5])
    ideal=np.array([-20, -12])

    problem = MOProblem(variables=varsl, objectives=[f1, f2], ideal=ideal, nadir=nadir)
    interface = NimbusInterface("COM3", button_pins=[2,3,4], potentiometer_pins=[0,1,2], variable_bounds= np.column_stack((ideal, nadir)))

    
    from desdeo_mcdm.utilities.solvers import solve_pareto_front_representation

    p_front = solve_pareto_front_representation(problem, step=1.0)[1]

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    from desdeo_mcdm.interactive.NIMBUS import NIMBUS

    method = NIMBUS(problem, "scipy_de")

    classification_request, plot_request = method.start()

    print(classification_request.content["message"]) # Divide objective functions
    
    objective_count = len(problem.objectives)
    classifications, levels = interface.get_classifications_and_levels(objective_count)
    solution_count = interface.specify_solution_count()

    response = {
        "classifications": classifications,
        "number_of_solutions": solution_count,
        "levels": levels
    }
    classification_request.response = response

    save_request, plot_request = method.iterate(classification_request)
    next_request = "save" # TEMP
    while True:
        if next_request == "save": # Then we need to specify indices for later viewing
            print(save_request.content['message'])
            objectives = save_request.content['objectives']
            print(objectives)
            saved_solutions = interface.save_solutions(objectives) # Get desired indices from the interface
            save_request.response = {"indices": saved_solutions}
            intermediate_request, plot_request = method.iterate(save_request)
            next_request = "intermediate"

        elif next_request == "intermediate": # See intermediate solutions?
            print(intermediate_request.content["message"])
            solutions = intermediate_request.content["solutions"]
            see_intermediate_solutions = interface.show_different_alternatives()
            if see_intermediate_solutions:
                sol = interface.choose_two_solutions(solutions)
                number_of_desired_solutions = interface.specify_solution_count()
                response = {"number_of_desired_solutions": number_of_desired_solutions, "indices": sol}
                intermediate_request.response = response
                save_request, plot_request = method.iterate(intermediate_request)
                next_request = "save"
            else:
                response = {"number_of_desired_solutions": 0, "indices": []}
                intermediate_request.response = response
                preferred_request, plot_request = method.iterate(intermediate_request)
                next_request = "preferred"
        
        elif next_request == "preferred":
            print(preferred_request.content["message"])
            solutions = preferred_request.content["solutions"]
            preferred_solution = interface.pick_preferred_solution(solutions)
            should_continue = interface.should_continue()
            preferred_request.response = {"index": preferred_solution, "continue": should_continue}
            if not should_continue:
                break
            classification_request, plot_request = method.iterate(preferred_request)
            next_request = "classification"
        
        elif next_request == "classification":
            print(classification_request.content["message"])
            objective_count = len(problem.objectives)
            classifications, levels = interface.get_classifications_and_levels(objective_count)
            solution_count = interface.specify_solution_count()
            response = {
                "classifications": classifications,
                "number_of_solutions": solution_count,
                "levels": levels
            }
            classification_request.response = response
            save_request, plot_request = method.iterate(classification_request)
            next_request = "save"

        plot(next_request)

    response = {
        "classifications": classifications,
        "number_of_solutions": solution_count,
        "levels": levels
    }
    classification_request.response = response


    stop_request, plot_request = method.iterate(preferred_request)

    print(f"Final decision variables: {stop_request.content['solution']}")

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    plt.scatter(stop_request.content["objective"][0], stop_request.content["objective"][1], label=f"final solution")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()