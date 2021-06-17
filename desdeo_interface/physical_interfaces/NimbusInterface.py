import os, sys
from desdeo_mcdm import interactive

from numpy.core.fromnumeric import var
from numpy.core.numeric import indices
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_interface.physical_interfaces.Interface import Interface
from desdeo_interface.components.Button import Button
from desdeo_interface.components.Potentiometer import Potentiometer
from desdeo_interface.components.Master import Master
from time import sleep
import numpy as np
from typing import Union, Optional, List, Tuple
from desdeo_problem.Problem import MOProblem

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
        # master: Master,
        problem: MOProblem,
        # button_pins: Union[np.array, List[int]] = [],
        # potentiometer_pins: Union[np.array, List[int]] = [],
        # rotary_encoders_pins: Union[np.ndarray, List[List[int]]] = [],
    ):
        super().__init__(problem, True)
    
    def get_levels(self):
        print("Set aspiration levels and/or upper bounds")
        return np.array(self.get_values(np.stack((self.problem.ideal, self.problem.nadir))))
    
    def get_classification(self) -> str:
        """
        Choose a classification for an objective from ["<", "<=", "=", ">=", "0"]

        Returns:
            str: Chosen classification
        """
        classification_options = ["<", "<=", "=", ">=", "0"]
        return self.choose_from(classification_options)[1]
    
    # Maybe this could be with value handlers aswell
    def get_classifications(self) -> List[str]:
        """
        Choose a classification for each objective 

        Returns:
            List[str]: Chosen classifications
        """
        classifications = []
        objective_count = self.problem.n_of_objectives
        for obj_index in range(objective_count):
            print(f"Pick a classification level for objective at index {obj_index}")
            classification = self.get_classification()
            classifications.append(classification)
        print(f"Chosen classifications: {classifications}")
        return classifications
    
    def specify_solution_count(self):
        """
        Specify the amount of solutions to be calculated in the next step

        Returns:
            int: The amount of solutions to be calculated
        """
        print("Select solution count")
        return self.choose_value(1, 4)
    
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
        return self.confirmation("Continue?")

    def try_another_classification(self):
        """
        Does the DM want to try another set of classifications

        Returns:
            bool: Whether or not to change classifications
        """
        return self.confirmation("Try another classification?")
    
    def show_different_alternatives(self):
        """
        Does the DM want to see different alternatives

        Returns:
            bool: Whether or not to see different alternatives
        """
        return self.confirmation("Show alternatives?")
    
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

    # def plot(request_type: str):
    #     plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    #     plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    #     plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    #     if request_type == "preferred":
    #         for i, z in enumerate(preferred_request.content["objectives"]):
    #             plt.scatter(z[0], z[1], label=f"solution {i}")
    #     elif request_type == "save":
    #         for i, z in enumerate(save_request.content["objectives"]):
    #             plt.scatter(z[0], z[1], label=f"solution {i}")
    #     else:
    #         for i, z in enumerate(intermediate_request.content["objectives"]):
    #             plt.scatter(z[0], z[1], label=f"solution {i}")
    #     plt.xlabel("f1")
    #     plt.ylabel("f2")
    #     plt.title("Approximate Pareto front of the Kursawe function")
    #     plt.legend()
    #     plt.show()
    
    def f_1(xs: np.ndarray):
        xs = np.atleast_2d(xs)
        xs_plusone = np.roll(xs, 1, axis=1)
        return np.sum(-10*np.exp(-0.2*np.sqrt(xs[:, :-1]**2 + xs_plusone[:, :-1]**2)), axis=1)

    def f_2(xs: np.ndarray):
        xs = np.atleast_2d(xs)
        return np.sum(np.abs(xs)**0.8 + 5*np.sin(xs**3), axis=1)

    f1 = _ScalarObjective(name="f1", evaluator=f_1)
    f2 = _ScalarObjective(name="f2", evaluator=f_2)

    nadir=np.array([-14, 0.5])
    ideal=np.array([-20, -12])

    varsl = variable_builder(
        ["x_1", "x_2", "x_3"],
        initial_values=[0, 0, 0],
        lower_bounds=[-5, -5, -5],
        upper_bounds=[5, 5, 5],
    )
    from desdeo_tools.solver import ScalarMethod
    from scipy.optimize import differential_evolution
    scalar_method = ScalarMethod(
        lambda x, _, **y: differential_evolution(x, **y), use_scipy=True, method_args={"polish": True, "disp": True}
    )

    problem = MOProblem(variables=varsl, objectives=[f1, f2], ideal=ideal, nadir=nadir)
    interface = NimbusInterface(problem)

    
    from desdeo_mcdm.utilities.solvers import solve_pareto_front_representation

    p_front = solve_pareto_front_representation(problem, step=1.0)[1]
    print(p_front)

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    from desdeo_mcdm.interactive.NIMBUS import NIMBUS

    method = NIMBUS(problem, scalar_method)

    classification_request, plot_request = method.start()


    print(classification_request.content["message"]) # Divide objective functions
    
    classifications = interface.get_classifications()
    levels = interface.get_levels()
    solution_count = interface.specify_solution_count()

    response = {
        "classifications": classifications,
        "number_of_solutions": solution_count,
        "levels": levels
    }

    print(response)
    classification_request.response = response

    save_request, plot_request = method.iterate(classification_request)
    next_request = "save"
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
            classifications = interface.get_classifications()
            levels = interface.get_levels()
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