import serial
import time

class GestureDetector:
    # Constants for gesture detection
    ACCELERATION_THRESHOLD = 800
    NO_GESTURE = 0
    GESTURE_ONE = 1  # Forward tilt
    GESTURE_TWO = 2  # Backward tilt

    def __init__(self, port='COM3', baudrate=9600):
        self.ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2)  # Wait for serial connection to stabilize
        self.previous_gesture = -1
        
        if self.ser.is_open:
            print("Connection is open")
        else:
            print("Failed to open connection")

    def update(self):
        current_gesture = self.detect_gesture()
        
        # Only print when gesture changes
        if current_gesture != self.previous_gesture:
            self.print_gesture(current_gesture)
            self.perform_gesture_action(current_gesture)
            self.previous_gesture = current_gesture

    def detect_gesture(self):
        if self.ser.in_waiting:
            # Read sensor data sent by the ATmega
            line = self.ser.readline().decode('utf-8').strip()
            try:
                ax, ay, az, gx, gy, gz = map(int, line.split(','))
                
                # Gesture recognition logic
                if ax > self.ACCELERATION_THRESHOLD and ay < self.ACCELERATION_THRESHOLD:
                    return self.GESTURE_ONE
                elif ax < -self.ACCELERATION_THRESHOLD and ay > self.ACCELERATION_THRESHOLD:
                    return self.GESTURE_TWO
                else:
                    return self.NO_GESTURE
            except ValueError:
                print("Received malformed data:", line)
        
        return self.NO_GESTURE

    def print_gesture(self, gesture):
        print("Detected Gesture: ", end="")
        if gesture == self.GESTURE_ONE:
            print("Forward Tilt")
        elif gesture == self.GESTURE_TWO:
            print("Backward Tilt")
        elif gesture == self.NO_GESTURE:
            print("None")

    def close(self):
        self.ser.close()
        print("Connection closed")

if __name__ == "__main__":
    gesture_detector = GestureDetector(port='COM3', baudrate=9600)
    
    try:
        while True:
            gesture_detector.update()
            time.sleep(0.05)  # Small delay to prevent overwhelming the output
            
    except KeyboardInterrupt:
        print("\nGesture detection stopped.")
    finally:
        gesture_detector.close()
