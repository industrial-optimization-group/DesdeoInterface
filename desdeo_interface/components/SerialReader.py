import serial
import ast

arduino = serial.Serial(port='COM3', baudrate=9600, timeout=.1)  # open serial port 
print("Serial port opened!")

def read():
    r = arduino.readline()[:-2] # Remove \r\n
    return r if len(r) > 0 else None

while True:
    d = read()
    if d:
        dict_str = d.decode("UTF-8")
        data = ast.literal_eval(dict_str)
        print(data)