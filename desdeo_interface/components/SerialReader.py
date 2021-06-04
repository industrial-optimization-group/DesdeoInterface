import serial
from serial.serialutil import SerialException
import serial.tools.list_ports
import ast
import threading
import time

class SerialReader:
    def __init__(self) -> None:
        ports = self.find("Arduino Uno")
        if len(ports) == 0:
            raise Exception("Couldn't find a usable board")
        self.uno = self.get(ports)
        self.prev_data = {}

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
            self.prev_data = data
            return data
        return self.prev_data





class Master:
    def __init__(self) -> None:
        self.updater = SerialReader()
        self.data = {}
        self.it = threading.Thread(target=self.update, daemon=True)
        self.it.start()

    def update(self):
        while True:
            self.data = self.updater.update()


m  = Master()
i = 0
while True:
    print(i)
    i += 1
    print(m.data)
    time.sleep(1)