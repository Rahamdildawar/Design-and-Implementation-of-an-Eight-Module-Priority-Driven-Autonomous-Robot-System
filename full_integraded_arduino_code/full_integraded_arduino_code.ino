// ====================================================================
// ALL 8 MODULES INTEGRATED - PRIORITY ARCHITECTURE MASTER CODE
// Board: Arduino Mega 2560
// FIXES APPLIED:
//   [FIX 1] ENA moved to pin 6 (PWM-capable). Pin 2 is INT0 on Mega.
//   [FIX 2] Flame suppression is now non-blocking (millis timer).
//   [FIX 3] pythonPotholeBrake has a 3s watchdog auto-clear.
//   [FIX 4] Ultrasonic pivot uses a counter to prevent re-trigger loops.
//   [FIX 5] Line sensor s2 check now correctly excludes s3.
//   [FIX 6] manualCommandTimeout extended to 1000ms to reduce chatter.
// ====================================================================

// [FIX 1] ENA moved from pin 2 to pin 6 — pin 2 is INT0, not PWM
#define ENA  6
#define IN1  3
#define IN2  4
#define IN3  7
#define IN4  8
#define ENB  5

// Module 8: Line Follower Inputs
#define LINE_S1  22
#define LINE_S2  23
#define LINE_S3  24
#define LINE_S4  25
#define LINE_S5  26

// Module 3: Flame Tracker Inputs
#define FLAME_L  30
#define FLAME_C  31
#define FLAME_R  32

// Module 1: Smoke/Gas Inputs
#define SMOKE_L  35
#define SMOKE_C  36
#define SMOKE_R  37

// Module 4: 3-Way Ultrasonic Array
#define TRIG_L   40
#define ECHO_L   41
#define TRIG_C   42
#define ECHO_C   43
#define TRIG_R   44
#define ECHO_R   45

// Peripheral Actuators
#define FAN_PIN     9
#define BUZZER_PIN  10

int currentSpeed = 160;
#define TURN_SPEED  140
#define SHARP_SPEED 170

// ── Priority Engine State ────────────────────────────────────────────

// [FIX 3] Pothole brake with watchdog: auto-clears after 3 seconds
bool pythonPotholeBrake     = false;
unsigned long potholeSetTime = 0;
#define POTHOLE_WATCHDOG_MS  3000UL

// Manual command state
char manualCommand          = 'S';
unsigned long manualCommandTimeout = 0;
// [FIX 6] Raised from 700ms to 1000ms to reduce command chaining noise
#define MANUAL_TIMEOUT_MS   1000UL

// [FIX 2] Flame suppression non-blocking timer
bool fanActive              = false;
unsigned long fanStartTime  = 0;
#define FAN_DURATION_MS     2000UL

// [FIX 4] Obstacle pivot counter to prevent re-trigger tight loops
int pivotCount              = 0;
#define MAX_PIVOT_ATTEMPTS  3

// ── Setup ────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(9600);   // Interface for Python CV Modules (6 & 5)
  Serial1.begin(9600);  // Interface for Bluetooth Module (7)

  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(LINE_S1, INPUT); pinMode(LINE_S2, INPUT); pinMode(LINE_S3, INPUT);
  pinMode(LINE_S4, INPUT); pinMode(LINE_S5, INPUT);
  pinMode(FLAME_L, INPUT); pinMode(FLAME_C, INPUT); pinMode(FLAME_R, INPUT);
  pinMode(SMOKE_L, INPUT); pinMode(SMOKE_C, INPUT); pinMode(SMOKE_R, INPUT);

  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_C, OUTPUT); pinMode(ECHO_C, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);

  pinMode(FAN_PIN, OUTPUT); pinMode(BUZZER_PIN, OUTPUT);

  stopMotors();
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("=== 8-MODULE PRIORITY ENGINE RUNNING ===");
}

// ── Main Loop ────────────────────────────────────────────────────────

void loop() {
  readControlInputs();  // Poll Module 6, 5, and 7 serial updates
  checkPotholeWatchdog(); // [FIX 3] Auto-clear stale pothole flag

  // ------------------------------------------------------------------
  // PRIORITY LEVEL 1: CRITICAL ENVIRONMENTAL SAFETY & ROAD ASSURANCE
  // ------------------------------------------------------------------
  bool smokeAlert = (digitalRead(SMOKE_L) == LOW ||
                     digitalRead(SMOKE_C) == LOW ||
                     digitalRead(SMOKE_R) == LOW);

  if (smokeAlert || pythonPotholeBrake) {
    stopMotors();
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(FAN_PIN, LOW);
    fanActive = false;  // Cancel any active fan cycle
    return;
  }
  digitalWrite(BUZZER_PIN, LOW);

  // ------------------------------------------------------------------
  // PRIORITY LEVEL 2: ACTIVE MISSION INCIDENT (MODULE 3: FIRE RECOVERY)
  // [FIX 2] Replaced delay(2000) with non-blocking millis() timer
  // ------------------------------------------------------------------
  bool flameL = (digitalRead(FLAME_L) == LOW);
  bool flameC = (digitalRead(FLAME_C) == LOW);
  bool flameR = (digitalRead(FLAME_R) == LOW);

  // If fan is currently running its suppression cycle, handle it here
  if (fanActive) {
    if (millis() - fanStartTime < FAN_DURATION_MS) {
      // Still suppressing — keep running, system remains responsive
      stopMotors();
      digitalWrite(FAN_PIN, HIGH);
      return;
    } else {
      // Suppression cycle complete
      fanActive = false;
      digitalWrite(FAN_PIN, LOW);
    }
  }

  if (flameL || flameC || flameR) {
    if (flameC) {
      stopMotors();
      // [FIX 2] Start non-blocking fan timer instead of delay(2000)
      digitalWrite(FAN_PIN, HIGH);
      fanActive    = true;
      fanStartTime = millis();
    } else if (flameL) {
      pivotLeft(TURN_SPEED);
      digitalWrite(FAN_PIN, LOW);
    } else if (flameR) {
      pivotRight(TURN_SPEED);
      digitalWrite(FAN_PIN, LOW);
    }
    return;
  }
  digitalWrite(FAN_PIN, LOW);

  // ------------------------------------------------------------------
  // PRIORITY LEVEL 3: ESCAPE SPATIAL BLOCKAGES (MODULE 4: ULTRASONIC)
  // [FIX 4] Pivot counter prevents endless re-trigger loops
  // ------------------------------------------------------------------
  long distC = getDistance(TRIG_C, ECHO_C);

  if (distC < 18 && distC > 0) {
    if (pivotCount < MAX_PIVOT_ATTEMPTS) {
      stopMotors();
      backward(currentSpeed, currentSpeed);
      delay(350);  // Short delay acceptable here (physical manoeuvre)
      stopMotors();

      long distL = getDistance(TRIG_L, ECHO_L);
      long distR = getDistance(TRIG_R, ECHO_R);
      if (distL > distR) pivotLeft(SHARP_SPEED);
      else               pivotRight(SHARP_SPEED);
      delay(500);
      stopMotors();

      pivotCount++;
    } else {
      // [FIX 4] Max pivots reached — stop and signal; await manual clear
      stopMotors();
      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);
    }
    return;
  } else {
    pivotCount = 0;  // Clear counter when path is open
  }

  // ------------------------------------------------------------------
  // PRIORITY LEVEL 4: INTERCEPT MANUAL COMMANDS (MODULE 5, 6, 7)
  // ------------------------------------------------------------------
  if (millis() < manualCommandTimeout) {
    executeManualAction(manualCommand);
    return;
  }

  // ------------------------------------------------------------------
  // PRIORITY LEVEL 5: DEFAULT LOCOMOTION ROUTINE (MODULE 8: LINE FOLLOWER)
  // ------------------------------------------------------------------
  runLineFollowingRoutine();
}

// ── Scheduler Data Ingestion ─────────────────────────────────────────

void readControlInputs() {
  if (Serial.available() > 0) {
    char dataByte = Serial.read();
    if (dataByte == 'P') {
      // [FIX 3] Record timestamp when pothole flag is set
      pythonPotholeBrake = true;
      potholeSetTime     = millis();
    } else if (dataByte == 'C') {
      pythonPotholeBrake = false;
    } else {
      manualCommand        = dataByte;
      // [FIX 6] Use defined constant for timeout
      manualCommandTimeout = millis() + MANUAL_TIMEOUT_MS;
    }
  }

  if (Serial1.available() > 0) {
    char dataByte = Serial1.read();
    if (dataByte >= '0' && dataByte <= '9') {
      currentSpeed = map(dataByte - '0', 0, 9, 0, 255);
    } else {
      manualCommand        = dataByte;
      manualCommandTimeout = millis() + MANUAL_TIMEOUT_MS;
    }
  }
}

// [FIX 3] Watchdog: auto-clear pothole flag if Python goes silent
void checkPotholeWatchdog() {
  if (pythonPotholeBrake &&
      (millis() - potholeSetTime > POTHOLE_WATCHDOG_MS)) {
    pythonPotholeBrake = false;
    Serial.println("WARN: Pothole flag auto-cleared by watchdog");
  }
}

// ── Line Following Routine ───────────────────────────────────────────

void runLineFollowingRoutine() {
  bool s1 = (digitalRead(LINE_S1) == LOW);
  bool s2 = (digitalRead(LINE_S2) == LOW);
  bool s3 = (digitalRead(LINE_S3) == LOW);
  bool s4 = (digitalRead(LINE_S4) == LOW);
  bool s5 = (digitalRead(LINE_S5) == LOW);

  if      (!s1 && !s2 && s3 && !s4 && !s5) forward(currentSpeed, currentSpeed); // Centre
  // [FIX 5] Added !s3 to soft-left check to avoid overlap with centre case
  else if (!s1 &&  s2 && !s3 && !s4 && !s5) forward(80, TURN_SPEED);            // Soft left
  else if ( s1)                              pivotLeft(SHARP_SPEED);             // Hard left
  else if (!s1 && !s2 && !s3 && s4 && !s5) forward(TURN_SPEED, 80);            // Soft right
  else if ( s5)                              pivotRight(SHARP_SPEED);            // Hard right
  else                                       forward(currentSpeed, currentSpeed); // Fallback
}

// ── Manual Action Executor ───────────────────────────────────────────

void executeManualAction(char cmd) {
  switch (cmd) {
    case 'F': forward(currentSpeed, currentSpeed);  break;
    case 'B': backward(currentSpeed, currentSpeed); break;
    case 'L': pivotLeft(TURN_SPEED);                break;
    case 'R': pivotRight(TURN_SPEED);               break;
    case 'S': stopMotors();                         break;
    default:  stopMotors();                         break; // Safety for unknown commands
  }
}

// ── Ultrasonic Helper ────────────────────────────────────────────────

long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 20000);
  return (dur == 0) ? 400 : (dur * 0.034) / 2;
}

// ── Motor Primitives ─────────────────────────────────────────────────

void forward(int l, int r) {
  analogWrite(ENA, l); analogWrite(ENB, r);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void backward(int l, int r) {
  analogWrite(ENA, l); analogWrite(ENB, r);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void pivotLeft(int spd) {
  analogWrite(ENA, spd); analogWrite(ENB, spd);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void pivotRight(int spd) {
  analogWrite(ENA, spd); analogWrite(ENB, spd);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
