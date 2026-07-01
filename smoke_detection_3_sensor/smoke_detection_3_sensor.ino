// ============================================================
//  Smoke-Following Robot — 3 Sensor Version
//  Motor Driver: L293D Module
//  Sensors: MQ-2 Smoke Sensors (DIGITAL - DOUT pin)
//  Extras:  Buzzer Alert, Tuned Turning, Fan trigger
// ============================================================

// --- MQ-2 Smoke Sensor DIGITAL pins (DOUT) ---
const int sensorLeft   = 10;
const int sensorCenter = 11;
const int sensorRight  = 12;

// --- L293D Module motor pins ---
// Motor A (Left wheel)
const int motorA_EN  = 9;
const int motorA_IN1 = 8;
const int motorA_IN2 = 7;

// Motor B (Right wheel)
const int motorB_EN  = 3;
const int motorB_IN3 = 5;
const int motorB_IN4 = 4;

// --- Extra output pins ---
const int BUZZER_PIN = 6;   // Buzzer alert
const int FAN_PIN    = 13;  // Fan relay or MOSFET

// --- Speed settings (0–255) ---
const int SPEED_FORWARD     = 170;
const int SPEED_TURN_FAST   = 200;  // Outer wheel sharp turn
const int SPEED_TURN_SLOW   = 70;   // Inner wheel gentle turn
const int SPEED_TURN_GENTLE = 150;  // Gentle outer wheel

// --- Lock-on: stop & activate fan when smoke centered long enough ---
const int LOCK_ON_COUNT = 10;  // ~500ms at 50ms loop
int lockOnTimer = 0;

// ============================================================
void setup() {
  Serial.begin(9600);

  // MQ-2 DOUT pins as input
  pinMode(sensorLeft,   INPUT);
  pinMode(sensorCenter, INPUT);
  pinMode(sensorRight,  INPUT);

  // Motor pins
  pinMode(motorA_EN,  OUTPUT);
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorB_EN,  OUTPUT);
  pinMode(motorB_IN3, OUTPUT);
  pinMode(motorB_IN4, OUTPUT);

  // Extra pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_PIN,    OUTPUT);

  stopMotors();
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(FAN_PIN,    LOW);

  // --- MQ-2 warm-up ---
  Serial.println("Warming up MQ-2 sensors... 20 seconds.");
  for (int i = 20; i > 0; i--) {
    Serial.print(i);
    Serial.println("s remaining...");
    delay(1000);
  }
  Serial.println("Smoke robot ready!");
}

// ============================================================
void loop() {
  // MQ-2 DOUT: LOW = smoke detected (threshold set by onboard pot)
  bool smokeLeft   = (digitalRead(sensorLeft)   == LOW);
  bool smokeCenter = (digitalRead(sensorCenter) == LOW);
  bool smokeRight  = (digitalRead(sensorRight)  == LOW);

  // Serial monitor for debugging
  Serial.print("L:"); Serial.print(smokeLeft);
  Serial.print("  C:"); Serial.print(smokeCenter);
  Serial.print("  R:"); Serial.println(smokeRight);

  // --- Buzzer: on if any smoke detected ---
  if (smokeLeft || smokeCenter || smokeRight) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // --- Lock-on logic ---
  // If only center sensor sees smoke for LOCK_ON_COUNT loops
  // → we are aligned with source → stop and ventilate
  if (smokeCenter && !smokeLeft && !smokeRight) {
    lockOnTimer++;
    Serial.print("Lock-on: "); Serial.println(lockOnTimer);

    if (lockOnTimer >= LOCK_ON_COUNT) {
      stopMotors();
      activateFan();
      delay(4000);      // Run fan for 4 seconds
      deactivateFan();
      lockOnTimer = 0;
      delay(500);
      return;
    }
    // Still approaching — keep moving forward
    moveForward();
    return;
  } else {
    lockOnTimer = 0;  // Reset if alignment lost
  }

  // --- Movement decisions ---
  if (smokeCenter && smokeLeft && smokeRight) {
    // Smoke from all directions — move straight
    moveForward();
  }
  else if (smokeCenter && smokeLeft) {
    // Center + left — gentle left correction
    gentleTurnLeft();
  }
  else if (smokeCenter && smokeRight) {
    // Center + right — gentle right correction
    gentleTurnRight();
  }
  else if (smokeCenter) {
    // Only center — go straight
    moveForward();
  }
  else if (smokeLeft && smokeRight) {
    // Both sides, no center — creep forward
    moveForwardSlow();
  }
  else if (smokeLeft) {
    // Only left — sharp left pivot
    sharpTurnLeft();
  }
  else if (smokeRight) {
    // Only right — sharp right pivot
    sharpTurnRight();
  }
  else {
    // No smoke — stop and wait
    stopMotors();
    deactivateFan();
  }

  delay(50);
}

// ============================================================
//  Motor control functions
// ============================================================

void moveForward() {
  analogWrite(motorA_EN, SPEED_FORWARD);
  analogWrite(motorB_EN, SPEED_FORWARD);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void moveForwardSlow() {
  analogWrite(motorA_EN, 110);
  analogWrite(motorB_EN, 110);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

// Gentle turn — both wheels forward, inner wheel slowed
void gentleTurnLeft() {
  analogWrite(motorA_EN, SPEED_TURN_SLOW);
  analogWrite(motorB_EN, SPEED_TURN_GENTLE);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void gentleTurnRight() {
  analogWrite(motorA_EN, SPEED_TURN_GENTLE);
  analogWrite(motorB_EN, SPEED_TURN_SLOW);
  digitalWrite(motorA_IN1, HIGH); digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

// Sharp turn — inner wheel reverses (pivot)
void sharpTurnLeft() {
  analogWrite(motorA_EN, SPEED_TURN_FAST);
  analogWrite(motorB_EN, SPEED_TURN_FAST);
  digitalWrite(motorA_IN1, LOW);  digitalWrite(motorA_IN2, HIGH);
  digitalWrite(motorB_IN3, HIGH); digitalWrite(motorB_IN4, LOW);
}

void sharpTurnRight() {
  analogWrite(motorA_EN, SPEED_TURN_FAST);
  analogWrite(motorB_EN, SPEED_TURN_FAST);
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
//  Fan / Buzzer control
// ============================================================

void activateFan() {
  digitalWrite(FAN_PIN, HIGH);
  Serial.println(">>> FAN ON — Ventilating!");
}

void deactivateFan() {
  digitalWrite(FAN_PIN, LOW);
  Serial.println(">>> FAN OFF");
}
