

import numpy as np

from desdeo_problem.Variable import variable_builder
from desdeo_problem.Objective import VectorObjective, _ScalarObjective
from desdeo_problem.Problem import MOProblem

from desdeo_mcdm.interactive import Nautilus

from physical_interfaces import NautilusInterface

if __name__ == "__main__":
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
    interface = NautilusInterface("COM3", [2,3,4], [0,1,2], np.column_stack((ideal, nadir)))

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
    
    """
    req = method.iterate(req)
    print("\nStep number: ", method._step_number)
    print("Iteration point: ", req.content["current_iteration_point"])
    print("Pareto optimal vector: ", method._fs[method._step_number])
    print("Lower bounds of objectives: ", req.content["lower_bounds"])
    # print("Upper bounds of objectives:", req.content["upper_bounds"])
    print("Closeness to Pareto optimal front", req.content["distance"])

    req.response = {
        "step_back": False,
        "short_step": False,
        "use_previous_preference": True,
    }

    # 2 - take a step back and give new preferences
    req = method.iterate(req)
    print("\nStep number: ", method._step_number)
    print("Iteration point: ", req.content["current_iteration_point"])
    print("Pareto optimal vector: ", method._fs[method._step_number])
    print("Lower bounds of objectives: ", req.content["lower_bounds"])
    print("Closeness to Pareto optimal front", req.content["distance"])

    req.response = {
        "step_back": True,
        "short_step": False,
        "use_previous_preference": False,
        "preference_method": 1,
        "preference_info": np.array([2, 3, 1]),
    }

    # 3 - give new preferences
    req = method.iterate(req)
    print("\nStep number: ", method._step_number)
    print("Iteration point: ", req.content["current_iteration_point"])
    print("Pareto optimal vector: ", method._fs[method._step_number])
    print("Lower bounds of objectives: ", req.content["lower_bounds"])
    print("Closeness to Pareto optimal front", req.content["distance"])

    req.response = {
        "step_back": False,
        "use_previous_preference": False,
        "preference_method": 1,
        "preference_info": np.array([1, 2, 1]),
    }

    # 4 - take a step back and provide new preferences
    req = method.iterate(req)
    print("\nStep number: ", method._step_number)
    print("Iteration point: ", req.content["current_iteration_point"])
    print("Pareto optimal vector: ", method._fs[method._step_number])
    print("Lower bounds of objectives: ", req.content["lower_bounds"])
    print("Closeness to Pareto optimal front", req.content["distance"])
    """


    """
    req.response = {
        "step_back": True,
        "short_step": False,
        "use_previous_preference": False,
        "preference_method": 2,
        "preference_info": np.array([30, 70]),
    }
    # 5. continue with the same preferences
    while method._n_iterations_left > 1:
        req = method.iterate(req)
        print("\nStep number: ", method._step_number)
        print("Iteration point: ", req.content["current_iteration_point"])
        print("Pareto optimal vector: ", method._fs[method._step_number])
        print("Lower bounds of objectives: ", req.content["lower_bounds"])
        print("Closeness to Pareto optimal front", req.content["distance"])
        req.response = {"step_back": False,
                        "use_previous_preference": True
                        }
    print("\nEnd of solution process")
    req = method.iterate(req)
    print(req.content)
    """