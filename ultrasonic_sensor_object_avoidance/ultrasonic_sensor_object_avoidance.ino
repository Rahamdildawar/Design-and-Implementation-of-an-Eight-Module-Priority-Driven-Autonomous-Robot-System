// ============================================================
//  Obstacle Avoidance Robot — 3 Ultrasonic Sensor Version
//  Motor Driver: L293D Module
//  Sensors: HC-SR04 Ultrasonic (Left, Center, Right)
// ============================================================

// --- Ultrasonic Sensor Pins ---
// Left sensor
const int TRIG_LEFT  = A0;
const int ECHO_LEFT  = A1;

// Center sensor
const int TRIG_CENTER = A2;
const int ECHO_CENTER = A3;

// Right sensor
const int TRIG_RIGHT  = A4;
const int ECHO_RIGHT  = A5;

// --- L293D Module motor pins ---
// Motor A (Left wheel)
const int motorA_EN  = 9;
const int motorA_IN1 = 8;
const int motorA_IN2 = 7;

// Motor B (Right wheel)
const int motorB_EN  = 3;
const int motorB_IN3 = 5;
const int motorB_IN4 = 4;

// --- Buzzer ---
const int BUZZER_PIN = 6;

// --- Distance thresholds (cm) ---
const int DANGER_DIST  = 15;  // Too close — must turn
const int WARNING_DIST = 30;  // Getting close — slow down
const int SAFE_DIST    = 40;  // All clear — full speed

// --- Speed settings (0–255) ---
const int SPEED_FULL   = 180;
const int SPEED_SLOW   = 110;
const int SPEED_TURN   = 160;
const int SPEED_BACKUP = 140;

// ============================================================
void setup() {
  Serial.begin(9600);

  // Ultrasonic pins
  pinMode(TRIG_LEFT,   OUTPUT);
  pinMode(ECHO_LEFT,   INPUT);
  pinMode(TRIG_CENTER, OUTPUT);
  pinMode(ECHO_CENTER, INPUT);
  pinMode(TRIG_RIGHT,  OUTPUT);
  pinMode(ECHO_RIGHT,  INPUT);

  // Motor pins
  pinMode(motorA_EN,  OUTPUT);
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorB_EN,  OUTPUT);
  pinMode(motorB_IN3, OUTPUT);
  pinMode(motorB_IN4, OUTPUT);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  stopMotors();
  Serial.println("Obstacle avoidance robot ready!");
}

// ============================================================
void loop() {
  // Read all 3 distances
  int distLeft   = getDistance(TRIG_LEFT,   ECHO_LEFT);
  int distCenter = getDistance(TRIG_CENTER, ECHO_CENTER);
  int distRight  = getDistance(TRIG_RIGHT,  ECHO_RIGHT);

  // Print for debugging
  Serial.print("L:"); Serial.print(distLeft);
  Serial.print("cm  C:"); Serial.print(distCenter);
  Serial.print("cm  R:"); Serial.println(distRight);

  // --- Buzzer: beep when any obstacle in danger zone ---
  if (distLeft < DANGER_DIST || distCenter < DANGER_DIST || distRight < DANGER_DIST) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ============================================================
  //  Decision logic
  // ============================================================

  // --- CASE 1: Center blocked ---
  if (distCenter < DANGER_DIST) {

    stopMotors();
    delay(200);
    moveBackward();
    delay(400);
    stopMotors();
    delay(200);

    // Choose turn direction based on which side has more space
    if (distLeft > distRight) {
      Serial.println("Center blocked → turning LEFT");
      sharpTurnLeft();
      delay(getTurnDelay(distLeft, distRight));
    } else if (distRight > distLeft) {
      Serial.println("Center blocked → turning RIGHT");
      sharpTurnRight();
      delay(getTurnDelay(distRight, distLeft));
    } else {
      // Both sides equally blocked — turn right by default
      Serial.println("All blocked → turning RIGHT (default)");
      sharpTurnRight();
      delay(600);
    }

    stopMotors();
    delay(100);
    return;
  }

  // --- CASE 2: Center warning zone — slow and steer ---
  if (distCenter < WARNING_DIST) {
    if (distLeft > distRight) {
      Serial.println("Center warning → gentle LEFT");
      gentleTurnLeft();
    } else {
      Serial.println("Center warning → gentle RIGHT");
      gentleTurnRight();
    }
    return;
  }

  // --- CASE 3: Left side blocked — steer right ---
  if (distLeft < DANGER_DIST) {
    Serial.println("Left blocked → steer RIGHT");
    gentleTurnRight();
    return;
  }

  // --- CASE 4: Right side blocked — steer left ---
  if (distRight < DANGER_DIST) {
    Serial.println("Right blocked → steer LEFT");
    gentleTurnLeft();
    return;
  }

  // --- CASE 5: Left warning — slight right correction ---
  if (distLeft < WARNING_DIST) {
    Serial.println("Left warning → slight RIGHT");
    slightTurnRight();
    return;
  }

  // --- CASE 6: Right warning — slight left correction ---
  if (distRight < WARNING_DIST) {
    Serial.println("Right warning → slight LEFT");
    slightTurnLeft();
    return;
  }

  // --- CASE 7: All clear — move forward ---
  Serial.println("All clear → forward");
  moveForward();

  delay(50);
}

// ============================================================
//  Ultrasonic distance function
// ============================================================

long getDistance(int trigPin, int echoPin) {
  // Send 10us pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse duration
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout

  // Convert to cm (speed of sound = 0.034 cm/us, divide by 2 for round trip)
  long distance = (duration * 0.034) / 2;

  // If timeout (no echo), return large safe value
  if (distance == 0 || distance > 400) {
    return 400;
  }

  return distance;
}

// ============================================================
//  Smart turn delay — longer if gap between sides is small
// ============================================================

int getTurnDelay(int openSide, int blockedSide) {
  int gap = openSide - blockedSide;
  if (gap > 20) return 350;   // Plenty of space — short turn
  if (gap > 10) return 500;   // Medium gap — medium turn
  return 700;                 // Tight gap — longer turn
}

// ============================================================
//  Motor control functions
// ============================================================

void moveForward() {
  analogWrite(motorA_EN, SPEED_FULL);
  analogWrite(motorB_EN, SPEED_FULL);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void moveForwardSlow() {
  analogWrite(motorA_EN, SPEED_SLOW);
  analogWrite(motorB_EN, SPEED_SLOW);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void moveBackward() {
  analogWrite(motorA_EN, SPEED_BACKUP);
  analogWrite(motorB_EN, SPEED_BACKUP);
  digitalWrite(motorA_IN1, LOW);  digitalWrite(motorA_IN2, HIGH);
  digitalWrite(motorB_IN3, LOW);  digitalWrite(motorB_IN4, HIGH);
}

// Sharp pivot turn — inner wheel reverses
void sharpTurnLeft() {
  analogWrite(motorA_EN, SPEED_TURN);
  analogWrite(motorB_EN, SPEED_TURN);
  digitalWrite(motorA_IN1, LOW);  digitalWrite(motorA_IN2, HIGH);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void sharpTurnRight() {
  analogWrite(motorA_EN, SPEED_TURN);
  analogWrite(motorB_EN, SPEED_TURN);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, LOW);  digitalWrite(motorB_IN4, HIGH);
}

// Gentle turn — both wheels forward, inner slowed
void gentleTurnLeft() {
  analogWrite(motorA_EN, 80);
  analogWrite(motorB_EN, SPEED_FULL);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void gentleTurnRight() {
  analogWrite(motorA_EN, SPEED_FULL);
  analogWrite(motorB_EN, 80);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

// Slight correction — barely slow one side
void slightTurnLeft() {
  analogWrite(motorA_EN, 130);
  analogWrite(motorB_EN, SPEED_FULL);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void slightTurnRight() {
  analogWrite(motorA_EN, SPEED_FULL);
  analogWrite(motorB_EN, 130);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void stopMotors() {
  analogWrite(motorA_EN, 0);
  analogWrite(motorB_EN, 0);
  digitalWrite(motorA_IN1, LOW); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, LOW); digitalWrite(motorB_IN4, LOW);
}
