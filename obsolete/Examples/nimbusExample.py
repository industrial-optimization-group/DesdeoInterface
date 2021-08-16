import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)
from desdeo_interface.physical_interfaces.NimbusInterface import NimbusInterface
import numpy as np
from desdeo_problem.Objective import _ScalarObjective
from desdeo_problem.Problem import MOProblem
from desdeo_problem.Variable import variable_builder
from desdeo_mcdm.interactive import NIMBUS
import matplotlib.pyplot as plt
from desdeo_problem.Problem import MOProblem
from desdeo_problem.Variable import variable_builder
from desdeo_problem.Objective import _ScalarObjective
if __name__ == "__main__":
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

    from desdeo_tools.solver import ScalarMethod
    from scipy.optimize import differential_evolution
    from desdeo_mcdm.interactive.NIMBUS import NIMBUS

    scalar_method = ScalarMethod(
        lambda x, _, **y: differential_evolution(x, **y), use_scipy=True, method_args={"polish": True, "disp": True}
    )

    method = NIMBUS(problem, scalar_method)

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

    classification_request, plot_request = method.start()
    print(classification_request.content["objective_values"])

    print(classification_request.content["message"]) # Divide objective functions
    
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