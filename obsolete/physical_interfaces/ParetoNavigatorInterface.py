import os, sys
p = os.path.abspath('.')
sys.path.insert(1, p)

from desdeo_problem.Variable import Variable
from desdeo_problem.Problem import MOProblem
from desdeo_interface.physical_interfaces.Interface import Interface
from time import sleep
import numpy as np
from typing import Union, Optional, List

class ParetoInterface(Interface):
    def __init__(
        self,
        problem: MOProblem,
    ):
        super().__init__(problem, True)

    def get_speed(self):
        return self.choose_value(1, 6)

    def get_referencepoint(self) -> np.ndarray:
        """
        Get a new referencepoint from the dm
        Returns:
            np.array: new reference point
        """ 
        return np.array(self.get_values(np.stack((self.problem.ideal, self.problem.nadir))))
    
    def get_classification(self) -> str:
        """
        Choose a classification for an objective from ["<", ">", "="]

        Returns:
            str: Chosen classification
        """
        classification_options = ["<", ">", "="]
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
            print(f"Pick a classification for objective at index {obj_index}")
            classification = self.get_classification()
            classifications.append(classification)
        print(f"Chosen classifications: {classifications}")
        return classifications

    def show_solution(self) -> int:
        return self.confirmation("Show solution?")
    
    def select_pref_method(self):
        return self.choose_from(np.array(["Reference point", "Classifications"]))
    
    def select_pref_solution(self, solutions):
        return self.choose_from(solutions)
    
    def change_speed(self):
        return self.confirmation("change speed?")
    
    def get_satisfaction(self):
        return self.confirmation("Satisfied?")
    
    def change_pref(self):
        return self.confirmation("Change preference information?")