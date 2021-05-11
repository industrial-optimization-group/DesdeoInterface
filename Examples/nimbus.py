from desdeo_mcdm import interactive
import numpy as np

import matplotlib.pyplot as plt
from desdeo_problem.Problem import MOProblem
from desdeo_problem.Variable import variable_builder
from desdeo_problem.Objective import _ScalarObjective

if __name__ == "__main__":
    from physical_interfaces import NimbusInterface

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
    interface = NimbusInterface("COM3", [2,3,4], [0,1,2], np.column_stack((nadir, ideal)))


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

    print(classification_request.content["message"])
    classifications, levels = interface.get_classifications_and_levels(2)

    response = {
        "classifications": classifications,
        "number_of_solutions": 3,
        "levels": [0, -5]
    }
    classification_request.response = response

    save_request, plot_request = method.iterate(classification_request)

    print(save_request.content.keys())
    print(save_request.content["message"])
    print(save_request.content["objectives"])

    response = {"indices": [0, 2]}
    save_request.response = response

    intermediate_request, plot_request = method.iterate(save_request)

    print(intermediate_request.content.keys())
    print(intermediate_request.content["message"])

    response = {"number_of_desired_solutions": 0, "indices": []}
    intermediate_request.response = response

    preferred_request, plot_request = method.iterate(intermediate_request)
    print(preferred_request.content.keys())
    print(preferred_request.content["message"])

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    for i, z in enumerate(preferred_request.content["objectives"]):
        plt.scatter(z[0], z[1], label=f"solution {i}")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    response = {"index": 1, "continue": True}
    preferred_request.response = response

    classification_request, plot_request = method.iterate(preferred_request)

    response = {
        "classifications": [">=", "<"],
        "number_of_solutions": 4,
        "levels": [-16, -1]
    }
    classification_request.response = response

    save_request, plot_request = method.iterate(classification_request)

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    for i, z in enumerate(save_request.content["objectives"]):
        plt.scatter(z[0], z[1], label=f"solution {i}")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    response = {"indices": [0, 1, 2, 3]}
    save_request.response = response

    intermediate_request, plot_request = method.iterate(save_request)

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    for i, z in enumerate(intermediate_request.content["objectives"]):
        plt.scatter(z[0], z[1], label=f"solution {i}")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    response = {
        "indices": [3, 4],
        "number_of_desired_solutions": 3,
        }
    intermediate_request.response = response

    save_request, plot_request = method.iterate(intermediate_request)

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    for i, z in enumerate(save_request.content["objectives"]):
        plt.scatter(z[0], z[1], label=f"solution {i}")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    response = {"indices": [1]}
    save_request.response = response

    intermediate_request, plot_request = method.iterate(save_request)

    response = {"number_of_desired_solutions": 0, "indices": []}
    intermediate_request.response = response

    preferred_request, plot_request = method.iterate(intermediate_request)

    plt.scatter(p_front[:, 0], p_front[:, 1], label="Pareto front")
    plt.scatter(problem.ideal[0], problem.ideal[1], label="Ideal")
    plt.scatter(problem.nadir[0], problem.nadir[1], label="Nadir")
    for i, z in enumerate(preferred_request.content["objectives"]):
        plt.scatter(z[0], z[1], label=f"solution {i}")
    plt.xlabel("f1")
    plt.ylabel("f2")
    plt.title("Approximate Pareto front of the Kursawe function")
    plt.legend()
    plt.show()

    response = {
        "index": 6,
        "continue": False,
    }

    preferred_request.response = response

    print("hello") #Nuis
    print(preferred_request) #Object

    stop_request, plot_request = method.iterate(preferred_request)

    print(stop_request) #Object

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