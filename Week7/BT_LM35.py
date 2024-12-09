import serial
import time
import matplotlib.pyplot as plt

# Define serial port and baud rate for Bluetooth communication
BLUETOOTH_PORT = 'COM6' 
BAUD_RATE = 9600

# Initialize Bluetooth serial communication
try:
    bt_serial = serial.Serial(BLUETOOTH_PORT, BAUD_RATE)
    print("Bluetooth initialized successfully.")
except Exception as e:
    print(f"Failed to initialize Bluetooth: {e}")
    exit()

def read_temperature():
    """
    Read temperature data from Arduino via Bluetooth.
    """
    try:
        if bt_serial.in_waiting > 0:
            data = bt_serial.readline().decode('utf-8').strip()
            if "Temperature:" in data:  # Check if the line contains the temperature label
                # Extract and convert the numeric value
                temperature_str = data.split("Temperature:")[-1].strip().replace("°C", "").strip()
                return float(temperature_str)
    except Exception as e:
        print(f"Error reading temperature: {e}")
        return None

def main():
    print("LM35 Bluetooth Data Transmission Starting...")

    # Initialize data for plotting
    temperatures = []
    timestamps = []
    plt.ion()
    fig, ax = plt.subplots()
    line, = ax.plot(timestamps, temperatures, label="Temperature (°C)")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Temperature (°C)")
    ax.set_title("Real-time LM35 Temperature Data")
    ax.legend()

    start_time = time.time()

    while True:
        try:
            # Read temperature from Arduino via Bluetooth
            temperature = read_temperature()
            if temperature is not None:
                # Display temperature on the console
                print(f"Temperature: {temperature:.2f} °C")

                # Update plot data
                current_time = time.time() - start_time
                timestamps.append(current_time)
                temperatures.append(temperature)

                # Update the plot
                line.set_xdata(timestamps)
                line.set_ydata(temperatures)
                ax.relim()
                ax.autoscale_view()
                plt.draw()
                plt.pause(0.01)

            # Wait before the next reading
            time.sleep(2)
        except KeyboardInterrupt:
            print("\nExiting...")
            bt_serial.close()
            plt.ioff()
            plt.show()
            break
        except Exception as e:
            print(f"Error: {e}")
            break

if __name__ == "__main__":
    main()