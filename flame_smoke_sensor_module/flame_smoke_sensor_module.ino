// MODULE 4: Hazard Mitigation & Fire Fan Vehicle
#define ENA  6
#define IN1  3
#define IN2  4
#define IN3  7
#define IN4  8
#define ENB  5

#define FLAME_L  30
#define FLAME_C  31
#define FLAME_R  32
#define SMOKE_C  36

#define FAN_PIN    9
#define BUZZER_PIN 10

void setup() {
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
  pinMode(FLAME_L, INPUT); pinMode(FLAME_C, INPUT); pinMode(FLAME_R, INPUT);
  pinMode(SMOKE_C, INPUT);
  pinMode(FAN_PIN, OUTPUT); pinMode(BUZZER_PIN, OUTPUT);
  stopMotors();
}

void loop() {
  bool smokeAlert = (digitalRead(SMOKE_C) == LOW);
  bool flameL = (digitalRead(FLAME_L) == LOW);
  bool flameC = (digitalRead(FLAME_C) == LOW);
  bool flameR = (digitalRead(FLAME_R) == LOW);

  if (smokeAlert) {
    stopMotors();               // Critical Leak: Full Safety Stop
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(FAN_PIN, LOW);
  } 
  else if (flameL || flameC || flameR) {
    if (flameC) {
      stopMotors();             // Lined up with fire: Halt vehicle
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(FAN_PIN, HIGH); // Turn on Fire Suppression Fan
    } else if (flameL) {
      pivotLeft(255);           // Fire is to the left: Pivot left at FULL SPEED
      digitalWrite(FAN_PIN, LOW);
    } else if (flameR) {
      pivotRight(255);          // Fire is to the right: Pivot right at FULL SPEED
      digitalWrite(FAN_PIN, LOW);
    }
  } 
  else {
    // Air Quality clear & No Fires: Drive forward normally at FULL SPEED
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(FAN_PIN, LOW);
    forward(255, 255);
  }
}

void forward(int l, int r) {
  analogWrite(ENA, l); analogWrite(ENB, r);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
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
