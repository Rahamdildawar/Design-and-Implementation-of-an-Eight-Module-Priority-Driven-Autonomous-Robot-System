// Motor 1 Pins (Left Motor)
const int motor1Pin1 = 5;
const int motor1Pin2 = 6;
const int enableLeft = 3;  // PWM pin for Left Motor Speed

// Motor 2 Pins (Right Motor)
const int motor2Pin1 = 9;
const int motor2Pin2 = 10;
const int enableRight = 11; // PWM pin for Right Motor Speed

char command; 
const int MAX_SPEED = 255; // Constant for 100% full speed

void setup() {
  // Initialize Hardware Serial on Pins 0 (RX) and 1 (TX) at 9600 baud.
  // This automatically sets up communication for Proteus or your HC-05.
  Serial.begin(9600);

  // Set motor control pins as outputs
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enableLeft, OUTPUT);
  
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enableRight, OUTPUT);

  // Initialize with motors off
  stopMotors();
}

void loop() {
  // Check for incoming data on Pins 0 and 1
  if (Serial.available() > 0) {
    command = Serial.read();

    switch (command) {
      case 'F': case 'f': moveForward(); break;
      case 'B': case 'b': moveBackward(); break;
      case 'L': case 'l': turnLeft(); break;
      case 'R': case 'r': turnRight(); break;
      case 'S': case 's': stopMotors(); break;
      default: break;
    }
  }
}

// ---- Movement Functions (Locked at Full Speed) ----

void moveForward() {
  analogWrite(enableLeft, MAX_SPEED);
  analogWrite(enableRight, MAX_SPEED);
  
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void moveBackward() {
  analogWrite(enableLeft, MAX_SPEED);
  analogWrite(enableRight, MAX_SPEED);

  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

void turnLeft() {
  analogWrite(enableLeft, MAX_SPEED);
  analogWrite(enableRight, MAX_SPEED);

  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
}

void turnRight() {
  analogWrite(enableLeft, MAX_SPEED);
  analogWrite(enableRight, MAX_SPEED);

  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
}

void stopMotors() {
  analogWrite(enableLeft, 0);
  analogWrite(enableRight, 0);

  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
}
