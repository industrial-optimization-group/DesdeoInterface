from desdeo_problem.Variable import Variable
import serial
from serial.serialutil import SerialException
import serial.tools.list_ports
import ast
import threading # Threading allows python to execute other code while waiting
import time
import numpy as np
from desdeo_problem.Objective import _ScalarObjective
from desdeo_problem import variable_builder

class SerialReader:
    def __init__(self) -> None:
        ports = self.find("Arduino Uno")
        if len(ports) == 0:
            raise Exception("Couldn't find a usable board")
        self.uno = self.get(ports)

    def read(self, port):
        r = port.readline()[:-2] # Remove \r\n
        return r if len(r) > 0 else None

    def find(self, s):
        ports = serial.tools.list_ports.comports()
        ss = [] 
        print("Found devices:")
        for port in ports:
            desc = port.description
            print(desc)
            if s in desc:
                ss.append(port.name)
        print()
        return ss

    def get(self, ports):
        for port in ports:
            try:
                uno = serial.Serial(port, 9600, timeout=.1)
                print(f"Port {port} is open")
                return uno
            except SerialException:
                print(f"Skipping port {port}")
        raise Exception("No available boards")
    
    def update(self):
        d = self.read(self.uno)
        if d:
            dict_str = d.decode("UTF-8")
            data = ast.literal_eval(dict_str)
            return data


from desdeo_problem.Problem import MOProblem

class InterFace:
    def __init__(self, problem: MOProblem = None, handle_objectives = False) -> None:
        self.ready = False
        self.problem = problem
        self.serial_reader = SerialReader()
        self.targets = {}
        self.construct(handle_objectives)
        self.updater = threading.Thread(target=self.update, daemon=True)
        self.updater.start()

    
    def construct(self, handle_objectives):
        print("Constructing the interface... This will take about 10 seconds")
        now = time.time()
        # Make sure every component has send data over master and master has written to serial
        while time.time() - now < 9:
            data = self.serial_reader.update()
        
        master = data.pop('master')
        values_to_handle = self.problem.objectives if handle_objectives else self.problem.variables
        p_count = len(data['P']) if 'P' in data else 0
        r_count = len(data['R']) if 'R' in data else 0

        if len(values_to_handle) > p_count + r_count:
            raise Exception("Not enough handlers")
        for component_type in data.keys():
            for node_id in data[component_type].keys():
                if len(values_to_handle) == 0: break
                self.assign_component(component_type, node_id, values_to_handle)
        print("succefully constructed the interface")
        self.ready = True
    
    def assign_component(self, component_type, node_id, values_to_handle):
        if component_type == "B":
            print("Skipping button")
            return
        next = values_to_handle.pop() # doesnt work with vector objectives
        if component_type == "P": print(f"Adding a potentiometer to {next.name}")
        if component_type == "R": print(f"Adding a rotary encoder to handle {next.name}")
        if isinstance(next, Variable):
            lower, upper = next.get_bounds()
        else:
            lower = next.lower_bound
            upper = next.upper_bound
        self.targets[next.name] = {
            'node': (component_type, node_id),
            'value': 0,
            'lower_bound': lower,
            'upper_bound': upper,
        }
    
    def get_value(self, target_name):
        return self.targets[target_name]
        
    def update(self):
        while True:
            if self.targets is None: break 
            new_data = self.serial_reader.update()
            for target in self.targets.values():
                component_type, node_id = target['node']
                value = new_data[component_type][node_id]
                u = target["upper_bound"]
                l = target["lower_bound"]
                if component_type == "P":
                    value = np.interp(value, [0,1023], [l,u])
                else:
                    if value > u: value = u
                    if value < l: value = l
                target["value"] = value



if __name__ == "__main__":
    # Objectives
    def f1(xs):
        xs = np.atleast_2d(xs)
        return -xs[:, 0] - xs[:, 1] + 5
    
    obj1 = _ScalarObjective("obj1", f1)
    objectives = [obj1]

    # variables
    var_names = ["x1", "x2"]
    initial_values = np.array([2, 3])

    lower_bounds = [0, 0]
    upper_bounds = [4, 6]
    bounds = np.stack((lower_bounds, upper_bounds))
    variables = variable_builder(var_names, initial_values, lower_bounds, upper_bounds)

    # problem
    problem = MOProblem(
        objectives=objectives, variables=variables
    )

    m  = InterFace(problem)
    while not m.ready:
        pass
    try:
        while True:
            for target in m.targets.items():
                print(target)
            time.sleep(1) # Threading is bad :[
            print()
    except KeyboardInterrupt:
        print("quitting...")
    
    quit()