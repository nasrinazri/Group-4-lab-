import serial

ser = serial.Serial('COM3', 9600)

try:
    while True:
        pot_value = ser.readline().decode().strip()
        print("Potentiometer Value:", pot_value)
except KeyboardInterrupt:
    ser.close()
    print("Serial connection closed.")
