import sys
import time
import serial

HEADER = bytes([0x7E, 0x7E])

def main():
    if len(sys.argv) != 3:
        print("Usage: py uploader.py <COM_PORT> <program.bin>")
        return

    port = sys.argv[1]
    program_file = sys.argv[2]

    with open(program_file, "rb") as f:
        data = f.read()

    size = len(data)
    packet = HEADER + bytes([size & 0xFF, (size >> 8) & 0xFF]) + data

    ser = serial.Serial(port, 115200, timeout=2)
    time.sleep(2)  # Arduino reset ke liye wait
    ser.reset_input_buffer()

    ser.write(packet)
    ser.flush()

    time.sleep(1)
    response = ser.read_all().decode(errors="ignore").strip()

    if response:
        print("Board response:", response)
    else:
        print("No response from board.")

    ser.close()

if __name__ == "__main__":
    main()