"""
A quick mockup I made (on my freetime: didn't waste my precious work hours) about an idea of an interface
you could have alongside the physical interface. This would be used to assign roles/objectives etc to each node.
Also it would allow the user to check that everything is correct. 

The code is not recommended to watch as it might hurt your brain or eyes.
proceed on your own risk.
"""

import serial
import time
from tkinter import *
import numpy as np
from serial.serialwin32 import Serial
ser = serial.Serial("COM3", 9600)

nodes = {}
configured = False

class Node:
    def __init__(self, pos, id, type) -> None:
        self.pos = pos
        self.id = int(id)
        self.type = type
        self.c = self.make()

    def make(self):
        if self.type == 1: color = 'green'
        elif self.type == 2: color = 'blue'
        elif self.type == 3: color = 'orange'
        elif self.type == 6: color = 'cyan'
        else: color = 'black'
        n = Canvas(
            c,
            width=100,
            height=100,
            bg = color
        )
        n.bind("<Button-1>", self.give_bounds)
        n.text = n.create_text(50,50, text="0")
        n.grid(column = self.pos[0], row = self.pos[1])
        return n
    
    def update_text(self, t):
        self.c.itemconfig(self.c.text, text = t)
    
    def disconnect(self):
        self.c.configure(bg="red")
    
    def destroy(self):
        self.c.destroy()
    
    def give_bounds(self, event):
        window = Toplevel(root)
        window.title("SET BOUNDS")
        window.geometry('300x200')
        min_t =  Text(window, height =2, width=20)
        min_t.insert(END, "min_value")
        min_t.pack()

        max_t = Text(window, height =2, width=20)
        max_t.insert(END, "max_value")
        max_t.pack()

        step_t = Text(window, height =2, width=20)
        step_t.insert(END, "Step_size")
        step_t.pack()

        def send():
            min_v = min_t.get("1.0", "end-1c")
            max_v = max_t.get("1.0", "end-1c")
            step_v = step_t.get("1.0", "end-1c")
            send_bounds(self, min_v, max_v, step_v)
            window.destroy()
        
        b = Button(window, text="Set bounds", command=send)
        b.pack()


def run_configuration():
    global configured
    global nodes
    b1.configure(text="Reconfigure", command=reconfigure)
    ser.write(b'S')
    poss = []
    ids = []
    types = []
    while True:
        r = ser.readline().decode("ascii")[:-2]
        s = r.split(" ")
        print(s)
 
        if s[0] == 'N':
            v = s[1].split(':')
            print(v)
            id = v[0]
            ids.append(id)
            t = int(v[1])
            types.append(t)
            dir = read_direction(v[-1])
            poss.append(dir)
        elif s[0] == 'O':
            print("Configuration complete")
            break
        else:
            print("IDK")
            break

    poss = np.array(poss)
    offset = np.abs(np.min(poss, axis = 0))
    poss += offset

    master = Node(offset, 254, 1)
    nodes[254] = master
    for i in range(len(ids)):
        node = Node(poss[i], ids[i], types[i])
        nodes[ids[i]] = node

    configured = True

def check_serial():
    global configured
    global nodes
    if configured:
        while ser.in_waiting:
            r = ser.readline().decode("ascii")[:-2]
            s = r.split(" ")
            if s[0] == 'V':
                v = s[1].split(':')
                nodes[v[0]].update_text(v[-1])
            elif s[0] == 'D':
                v = s[1].split(':')
                pos = nodes[v[0]].pos + read_direction(v[1])
                node = find_with(pos)
                if node is not None:
                    node.disconnect()
            elif s[0] == 'C':
                reconfigure()

    root.after(100, check_serial)
        
def reconfigure():
    global configured
    global nodes
    print("DESTROY")
    ser.write(b'Q')
    configured = False
    for id, node in nodes.items():
        node.destroy()
    nodes = {}        
    root.after(500, run_configuration)

def find_with(pos):
    for id, node in nodes.items():
        if np.all(node.pos == pos):
            return nodes[id]
    return None

def send_bounds(node,min, max, step = 0):
    if node.type == 1:
        s = "P:0"
    else: s = "R:0"
    bounds = f"B {node.id}:{s}:{min}:{max}:{step}\r\n"
    bounds = bounds.encode('ascii')
    print(bounds)
    ser.write(bounds)

def read_direction(s):
    dir = [0,0]
    for d in s:
        if d == '0':
            dir[1] -= 1
        elif d == '1':
            dir[0] += 1
        elif d == '2':
            dir[1] += 1
        elif d == '3':
            dir[0] -= 1
        else:
            print("what is this direction")
    return dir

while ser.readline() is None:
    continue

root = Tk("Main window")
root.geometry("600x600")

c = Canvas(
    root,
    width= 500,
    height= 500,
)

c.place(relx=0.5, rely=0.5, anchor=CENTER)

b1 = Button(
    root,
    text="Run configuration",
    command=run_configuration)
b1.pack(side=BOTTOM, pady=10)

root.after(3000, check_serial)
root.mainloop()