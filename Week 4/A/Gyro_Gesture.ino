#include <Wire.h>
#include <MPU6050.h>

// Class to handle gesture detection using MPU6050
class GestureDetector {
private:
    MPU6050 mpu;
    int previousGesture;
    
    // Gesture detection thresholds
    static const int ACCELERATION_THRESHOLD = 800;  // Sensitivity
    
    // Gesture types
    static const int NO_GESTURE = 0;
    static const int GESTURE_ONE = 1;  // Forward tilt
    static const int GESTURE_TWO = 2;  // Backward tilt

public:
    GestureDetector() : previousGesture(-1) {}
    
    void begin() {
        Wire.begin();
        mpu.initialize();
        
        // Verify connection
        if (!mpu.testConnection()) {
            Serial.println("MPU6050 connection failed!");
            while (1);
        }
        Serial.println("MPU6050 connection successful!");
    }
    
    void update() {
        int currentGesture = detectGesture();
        
        // Gesture Update
        if (currentGesture != previousGesture) {
            printGesture(currentGesture);
            performGestureAction(currentGesture);
            previousGesture = currentGesture;
        }
    }

private:
    int detectGesture() {
        int16_t ax, ay, az;    // Acceleration values
        int16_t gx, gy, gz;    // Gyroscope values
        
        // Get raw sensor data
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        
        // Gesture recognition logic
        if (ax > ACCELERATION_THRESHOLD && ay < ACCELERATION_THRESHOLD) {
            return GESTURE_ONE;
        } 
        else if (ax < -ACCELERATION_THRESHOLD && ay > ACCELERATION_THRESHOLD) {
            return GESTURE_TWO;
        }
        
        return NO_GESTURE;
    }
    
    void printGesture(int gesture) {
        Serial.print("Detected Gesture: ");
        switch (gesture) {
            case GESTURE_ONE:
                Serial.println("Forward Tilt");
                break;
            case GESTURE_TWO:
                Serial.println("Backward Tilt");
                break;
            case NO_GESTURE:
                Serial.println("None");
                break;
        }
    }
};

// Global instance of gesture detector
GestureDetector gestureDetector;

void setup() {
    Serial.begin(9600);
    gestureDetector.begin();
}

void loop() {
    gestureDetector.update();
    delay(50);  // Delay to prevent serial clot
}
