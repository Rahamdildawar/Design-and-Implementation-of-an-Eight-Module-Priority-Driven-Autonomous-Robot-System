// --- MOTOR DRIVER L293D PIN MAPPING ---
// Left Motor (Motor A)
const int enA = 9;  // Forced to Digital ON
const int in1 = 8;  // Direction 1
const int in2 = 7;  // Direction 2

// Right Motor (Motor B)
const int enB = 3;  // Forced to Digital ON
const int in3 = 4;  // Direction 3
const int in4 = 5;  // Direction 4

void setup() {
  // Configure all H-Bridge pins as Output
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  // Initialize Serial Link matching your Python BAUD_RATE
  Serial.begin(9600);
  
  // Safe Default state
  emergencyStop();
}

void loop() {
  // Listen for incoming pipeline packages from Python via COMPIM
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 'F': // Pipeline Command: Move Forward
        executeForward();
        break;
        
      case 'B': // Pipeline Command: Move Backward
        executeBackward();
        break;
        
      case 'L': // Pipeline Command: Spin Left
        executeLeftTurn();
        break;
        
      case 'R': // Pipeline Command: Spin Right
        executeRightTurn();
        break;
        
      case 'H': // Pothole Filter Active: Hard Stop (Hazard)
      case 'S': // Manual Control Override: Standard Stop
      default:  // Catch-all safety fallback
        emergencyStop();
        break;
    }
  }
}

// -------------------------------------------------------------
// HARDWARE DRIVER EXECUTION LOGIC (FORCED FULL POWER)
// -------------------------------------------------------------

void executeForward() {
  // Set directions
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

  // FIX: Force both Enable pins to steady 5V logic HIGH to run both motors at full speed
  digitalWrite(enA, HIGH);
  digitalWrite(enB, HIGH);
}

void executeBackward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  digitalWrite(enA, HIGH);
  digitalWrite(enB, HIGH);
}

void executeLeftTurn() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

  digitalWrite(enA, HIGH);
  digitalWrite(enB, HIGH);
}

void executeRightTurn() {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  digitalWrite(enA, HIGH);
  digitalWrite(enB, HIGH);
}

void emergencyStop() {
  // Immediately drop enable logic to ground
  digitalWrite(enA, LOW);
  digitalWrite(enB, LOW);
  
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
