import numpy as np
from desdeo_mcdm.interactive.InteractiveMethod import InteractiveMethod
from desdeo_problem.Objective import VectorObjective, _ScalarObjective
from desdeo_problem.Problem import MOProblem
from desdeo_problem.Variable import variable_builder

from desdeo_mcdm.interactive import ReferencePointMethod
from physical_interfaces import RPMInterface
"""
Reference Point Method (RPM)
"""




# testing the method
if __name__ == "__main__":
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


    # problem
    prob = MOProblem(objectives=[obj1, obj2, obj3], variables=variables)  # objectives "seperately"

    # solved in Nautilus.py
    ideal = np.array([-6.34, -3.44487179, -7.5])
    nadir = np.array([-4.751, -2.86054116, -0.32111111])

    # interface
    interface = RPMInterface("COM3", [2,3,4], [0,1,2], np.column_stack((ideal, nadir)))

    # start solving
    method = ReferencePointMethod(problem=prob, ideal=ideal, nadir=nadir)

    # Pareto optimal solution: [-6.30, -3.26, -2.60, 3.63]

    print("Let's start solving\n")
    print(f"nadir: {nadir}")
    print(f"ideal: {ideal}\n")
    
    req = method.start()
    rp = interface.get_input()
    req.response = {
        "reference_point": rp,
    }

    print(f"Step number: {method._h} \n")

    # 1 - continue with same preferences
    req = method.iterate(req)
    step = 1
    print("\nStep number: ", method._h)
    print("Reference point: ", rp)
    print("Pareto optimal solution: ", req.content["current_solution"])
    print("Additional solutions: ", req.content["additional_solutions"])

    satisfied = interface.get_satisfaction()
    while not satisfied:
        step += 1
        rp = rp = interface.get_input() #Instead let me choose the next rp
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