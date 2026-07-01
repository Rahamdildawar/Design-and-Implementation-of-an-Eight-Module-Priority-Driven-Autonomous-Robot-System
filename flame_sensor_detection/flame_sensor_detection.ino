 // ============================================================
//  Flame-Following Robot — 3 Sensor Version
//  Motor Driver: L293D Module
//  Sensors: DIGITAL IR Flame Sensors (LOW = flame detected)
//  Extras:  Extinguishing Fan, Tuned Turning
// ============================================================

// --- Digital Flame sensor pins ---
const int sensorLeft   = 10;
const int sensorCenter = 11;
const int sensorRight  = 12;

// --- L293D Module motor pins ---
// Motor A (Left)
const int motorA_EN  = 9;
const int motorA_IN1 = 8;
const int motorA_IN2 = 7;

// Motor B (Right)
const int motorB_EN  = 3;
const int motorB_IN3 = 5;
const int motorB_IN4 = 4;

// --- Extinguishing fan pin ---
const int FAN_PIN = 6;  // PWM pin — connect relay or MOSFET here

// --- Speed settings (0–255) ---
const int SPEED_FORWARD     = 180;  // Normal forward speed
const int SPEED_TURN_FAST   = 200;  // Outer wheel during sharp turn
const int SPEED_TURN_SLOW   = 80;   // Inner wheel during sharp turn (tuned)
const int SPEED_TURN_GENTLE = 150;  // Both wheels, gradual correction

// --- Fan control ---
const int FAN_SPEED_FULL = 255;     // Full blast when at flame
const int FAN_SPEED_OFF  = 0;

// --- How close before we stop and extinguish ---
// Since digital sensors don't give distance,
// we use a "lock-on" counter: if center flame
// is seen for N consecutive loops, stop and extinguish
const int LOCK_ON_COUNT = 8;        // ~400ms at 50ms loop
int lockOnTimer = 0;

// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(sensorLeft,   INPUT);
  pinMode(sensorCenter, INPUT);
  pinMode(sensorRight,  INPUT);

  pinMode(motorA_EN,  OUTPUT);
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorB_EN,  OUTPUT);
  pinMode(motorB_IN3, OUTPUT);
  pinMode(motorB_IN4, OUTPUT);

  pinMode(FAN_PIN, OUTPUT);

  stopMotors();
  fanOff();
  Serial.println("Flame robot ready.");
}

// ============================================================
void loop() {
  // Digital sensors: LOW = flame detected
  bool flameLeft   = (digitalRead(sensorLeft)   == LOW);
  bool flameCenter = (digitalRead(sensorCenter) == LOW);
  bool flameRight  = (digitalRead(sensorRight)  == LOW);

  Serial.print("L:"); Serial.print(flameLeft);
  Serial.print("  C:"); Serial.print(flameCenter);
  Serial.print("  R:"); Serial.println(flameRight);

  // --- Lock-on logic: center flame seen long enough → extinguish ---
  if (flameCenter && !flameLeft && !flameRight) {
    lockOnTimer++;
    Serial.print("Lock-on: "); Serial.println(lockOnTimer);

    if (lockOnTimer >= LOCK_ON_COUNT) {
      // We're aligned and close enough — stop and blast fan
      stopMotors();
      fanOn();
      delay(3000);   // Run fan for 3 seconds
      fanOff();
      lockOnTimer = 0;
      delay(500);
      return;        // Skip rest of loop, re-read sensors
    }
    // Not locked on yet — keep moving forward
    moveForward();
    return;
  }
  else {
    lockOnTimer = 0;  // Reset if center flame lost
  }

  // --- Movement decisions ---
  if (flameCenter && flameLeft && flameRight) {
    // All three: flame straight ahead, move forward
    moveForward();
    fanOff();
  }
  else if (flameCenter && flameLeft) {
    // Center + left: gentle left correction
    gentleTurnLeft();
    fanOff();
  }
  else if (flameCenter && flameRight) {
    // Center + right: gentle right correction
    gentleTurnRight();
    fanOff();
  }
  else if (flameCenter) {
    // Only center: move straight
    moveForward();
    fanOff();
  }
  else if (flameLeft && flameRight) {
    // Both sides, no center: move forward slowly
    moveForwardSlow();
    fanOff();
  }
  else if (flameLeft) {
    // Only left: sharp left turn
    sharpTurnLeft();
    fanOff();
  }
  else if (flameRight) {
    // Only right: sharp right turn
    sharpTurnRight();
    fanOff();
  }
  else {
    // No flame detected: stop
    stopMotors();
    fanOff();
  }

  delay(50);
}

// ============================================================
//  Motor control — tuned turning
// ============================================================

void moveForward() {
  analogWrite(motorA_EN, SPEED_FORWARD);
  analogWrite(motorB_EN, SPEED_FORWARD);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void moveForwardSlow() {
  analogWrite(motorA_EN, 120);
  analogWrite(motorB_EN, 120);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

// Gentle turns: both wheels forward, outer faster
// Used when center + one side sensor fires (small correction needed)
void gentleTurnLeft() {
  analogWrite(motorA_EN, SPEED_TURN_SLOW);    // Left wheel slower
  analogWrite(motorB_EN, SPEED_TURN_GENTLE);  // Right wheel normal
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void gentleTurnRight() {
  analogWrite(motorA_EN, SPEED_TURN_GENTLE);  // Left wheel normal
  analogWrite(motorB_EN, SPEED_TURN_SLOW);    // Right wheel slower
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

// Sharp turns: inner wheel reverses, outer goes forward
// Used when only left or only right sensor fires (big correction needed)
void sharpTurnLeft() {
  analogWrite(motorA_EN, SPEED_TURN_FAST);  // Left wheel backward
  analogWrite(motorB_EN, SPEED_TURN_FAST);  // Right wheel forward
  digitalWrite(motorA_IN1, LOW);  digitalWrite(motorA_IN2, HIGH);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void sharpTurnRight() {
  analogWrite(motorA_EN, SPEED_TURN_FAST);  // Left wheel forward
  analogWrite(motorB_EN, SPEED_TURN_FAST);  // Right wheel backward
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, LOW);  digitalWrite(motorB_IN4, HIGH);
}

void stopMotors() {
  analogWrite(motorA_EN, 0);
  analogWrite(motorB_EN, 0);
  digitalWrite(motorA_IN1, LOW); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, LOW); digitalWrite(motorB_IN4, LOW);
}

// ============================================================
//  Fan control
// ============================================================

void fanOn() {
  analogWrite(FAN_PIN, FAN_SPEED_FULL);
  Serial.println(">>> FAN ON — Extinguishing!");
}

void fanOff() {
  analogWrite(FAN_PIN, FAN_SPEED_OFF);
}
