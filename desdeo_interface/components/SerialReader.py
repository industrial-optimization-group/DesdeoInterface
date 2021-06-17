from os import startfile
import serial
from serial.serialutil import SerialException
import serial.tools.list_ports
import ast
import numpy as np

class SerialReader:
    def __init__(self, crc_key: np.uint8 = 7) -> None:
        self._data = {}
        self._buffer = ''
        ports = self.find("Arduino Uno")
        if len(ports) == 0:
            raise Exception("Couldn't find a usable board")
        self._port = self.open(ports)
        self.start_communication()
        self.crc_lookup_table = self.create_lookup_table(crc_key)
    
    def create_lookup_table(self, key: np.uint8):
        table = []
        for i in range(256):
            cur: np.uint8 = i
            for _ in range(8):
                if (np.bitwise_and(cur, 0x80)) != 0:
                    cur = np.left_shift(cur, 1) % 256
                    cur = np.bitwise_xor(cur, key)
                else:
                    cur = np.left_shift(cur, 1) % 256
            table.append(cur)
        return table

    def crc_check(self, data: str, crc: np.uint8) -> bool:
        remainder = 0
        data_arr = [ord(s) for s in data] + [crc]
        for d in data_arr:
            remainder = self.crc_lookup_table[d ^ remainder]
        return remainder == 0

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

    def open(self, ports):
        for port in ports:
            try:
                uno = serial.Serial(port, 9600, timeout=.1)
                if not uno.isOpen(): uno.open()
                print(f"Port {port} is open")
                return uno
            except SerialException:
                print(f"Skipping port {port}")
        raise Exception("No available boards")
    
    def update(self):
        try:
            self._buffer += self._port.read(self._port.inWaiting()).decode("ascii")
        except UnicodeDecodeError:
            print("Failed to decode a byte")
            return
        if '\n' in self._buffer:
            d, self._buffer = self._buffer.split('\n')[-2:]
            try:
                dict_str, crc = d.rsplit('}', 1)
                dict_str = dict_str + '}'
                if not self.crc_check(dict_str, int(crc)):
                    raise Exception("CRC8 checksum doesn't match")
                data = ast.literal_eval(dict_str)
                if data is not None:
                    self._data.update(data)
            except Exception as e:
                print("Couldn't parse data")
                print(f"got exception {e}")
    
    def start_communication(self):
        self._port.write('R'.encode("ascii"))
        if self._port.read().decode('ascii') == '': self.start_communication()
    
    def end_communication(self):
        self._port.write(b'Q')
        self._port.close()
        print("Port closed")

    def data(self):
        temp = self._data.copy()
        return temp


if __name__ == "__main__":
    s = SerialReader()
    try:
        while True:
            s.update()
            print(s.data())
    except KeyboardInterrupt: print('quittin')