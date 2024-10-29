import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import serial
from time import time
import re

class PotentiometerVisualizer:
    def __init__(self, port='COM6', baudrate=9600, max_points=100):
        try:
            # Set up serial connection
            self.serial = serial.Serial(
                port=port,
                baudrate=baudrate,
                timeout=0.1,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE
            )
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
        except serial.SerialException as e:
            print(f"Error opening serial port: {e}")
            raise

        # Initialize data
        self.max_points = max_points
        self.times = np.array([])  # Time data points
        self.values = np.array([])  # Potentiometer angle values

        # Create the plot
        self.fig, self.ax = plt.subplots(figsize=(10, 6))
        self.line, = self.ax.plot([], [], color='pink', label='Servo Angle', linewidth=2)
        self.ax.set_title('Real-time Servo Angle Readings')
        self.ax.set_xlabel('Time (seconds)')
        self.ax.set_ylabel('Angle (degrees)')
        self.ax.grid(True)
        self.ax.legend()
        self.ax.set_ylim(0, 180)  # Angle range for potentiometer

        # Start time for tracking time offsets
        self.start_time = time()

    def read_angle(self):
        """Reads the potentiometer angle from the serial input."""
        try:
            if self.serial.in_waiting > 0:
                raw_data = self.serial.readline()
                line = raw_data.decode().strip()
                match = re.search(r'Servo Angle:\s*(\d+)', line)
                if match:
                    return float(match.group(1))
                else:
                    return None
        except (serial.SerialException, UnicodeDecodeError) as e:
            print(f"Serial/Decode error: {e}")
            return None

    def update(self, frame):
        """Update function to add new data points to the graph."""
        current_time = time() - self.start_time
        current_value = self.read_angle()
        
        if current_value is not None:
            self.times = np.append(self.times, current_time)
            self.values = np.append(self.values, current_value)
            
            if len(self.times) > self.max_points:
                self.times = self.times[-self.max_points:]
                self.values = self.values[-self.max_points:]

            # Update plot line data
            self.line.set_data(self.times, self.values)
            self.ax.set_xlim(max(0, self.times[-1] - 10), self.times[-1] + 2)

        return self.line,

    def run(self):
        """Starts the real-time plot animation."""
        anim = FuncAnimation(self.fig, self.update, frames=None, interval=50, blit=False, cache_frame_data=False)

        plt.show()

    def cleanup(self):
        """Closes the serial port upon completion."""
        if self.serial.is_open:
            self.serial.close()
            print("Serial connection closed")

def main():
    visualizer = PotentiometerVisualizer(port='COM6')
    try:
        visualizer.run()
    except KeyboardInterrupt:
        print("Visualization interrupted by user.")
    finally:
        visualizer.cleanup()

if __name__ == "__main__":
    main()
