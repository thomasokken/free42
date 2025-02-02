import time

import serial
import serial.tools.list_ports

# location on my pc: stm-workspace\Free42\Debug\Free42.bin

print("Flash loader 2.0\n")

try:
    file = open(input("Enter file to load to flash: "), 'rb')
except FileNotFoundError:
    print("File not found")
    exit()

data = file.read()
print(len(data), "bytes to write")

print("\nAvailable Ports:")
ports = serial.tools.list_ports.comports()
for port in ports:
    print('\t' + port.device)
port = input("Enter the port name: ")
ser = serial.Serial(port, 9600, timeout=1)

ser.write('frw\nwe\nec\n'.encode())
for i in range(20):
    print("Erasing chip: ", i * 5, "%")
    time.sleep(1)
    
ser.close()

print('Disconnect and reconnect power to chip while holding down top left button')


def print_status(current, total):
    print(f'Writing program: {current / total * 100:.1f}%')
    
print('New ports:')
ports = serial.tools.list_ports.comports()
for port in ports:
    print('\t' + port.device)
port = input("Enter the port name: ")
ser = serial.Serial(port, 9600, timeout=1)

ser.write('echo off\nfrw\n'.encode())

size = 256
sb = True
count = 0
for i in range(0, len(data), size):
    if i < 299500:
        continue

    if count == 20:
        print_status(i - 299500, len(data) - 295000)
        count = 0
    d = data[i:i+size]
    if len(d) < size:
        d += b'\xFF' * (size - len(d))
    ser.write(f'W {i} {size} \n'.encode())
    time.sleep(0.001)
    ser.write(d)
    time.sleep(0.05)
    count += 1

print_status(1,1)

ser.write('exit\necho on\n'.encode())
ser.close()

port = input('Upload paused. Enter port once ready to continue:')

ser = serial.Serial(port, 9600, timeout=1)
ser.write('echo off\nfrw\n'.encode())

size = 256
small_count = 0
sb = True
for i in range(0, len(data), size):
    if i > 300000:
        break
    d = data[i:i+size]
    if len(d) < size:
        d += b'\xFF' * (size - len(d))
    ser.write(f'W {i} {size} \n'.encode())
    if count == 20:
        print_status(i, 300000)
        count = 0
    time.sleep(0.001)
    ser.write(d)
    time.sleep(0.05)

    count += 1

ser.write('exit\necho on\n'.encode())
ser.close()
