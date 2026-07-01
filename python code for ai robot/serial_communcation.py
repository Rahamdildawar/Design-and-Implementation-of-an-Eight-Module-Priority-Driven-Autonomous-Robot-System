import serial
import time

# Open port COM10 at 9600 baud
ser = serial.Serial('COM11', 9600, timeout=1)

# Allow the virtual connection to fully establish
time.sleep(2)
ser.reset_input_buffer()
ser.reset_output_buffer()

print("Connection established. Sending commands...")

try:
    while True:
        # Send Forward command
        ser.write('F'.encode('utf-8'))
        print("Sent: Forward")
        time.sleep(2)

        # Send Stop command
        ser.write('S'.encode('utf-8'))
        print("Sent: Stop")
        time.sleep(2)

except KeyboardInterrupt:
    # Safely close the port if you press Ctrl+C
    ser.close()
    print("\nSerial port closed.")