// MODULE 3: Autonomous Line Following Vehicle
#define ENA  6
#define IN1  3
#define IN2  4
#define IN3  7
#define IN4  8
#define ENB  5

#define LINE_S1  22
#define LINE_S2  23
#define LINE_S3  24
#define LINE_S4  25
#define LINE_S5  26

void setup() {
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  
  pinMode(LINE_S1, INPUT); pinMode(LINE_S2, INPUT); pinMode(LINE_S3, INPUT);
  pinMode(LINE_S4, INPUT); pinMode(LINE_S5, INPUT);

  // --- PROTEUS SIMULATION SPEED BOOST ---
  // Sets pre-scalers to 1 to prevent Proteus engine lag on PWM pins 5 & 6
  TCCR3B = (TCCR3B & 0xF8) | 0x01; 
  TCCR4B = (TCCR4B & 0xF8) | 0x01; 

  stopMotors();
}

void loop() {
  bool s1 = (digitalRead(LINE_S1) == LOW);
  bool s2 = (digitalRead(LINE_S2) == LOW);
  bool s3 = (digitalRead(LINE_S3) == LOW);
  bool s4 = (digitalRead(LINE_S4) == LOW);
  bool s5 = (digitalRead(LINE_S5) == LOW);

  // All states are now mapped to absolute full throttle (255)
  if (!s1 && !s2 && s3 && !s4 && !s5) {
    forward(255, 255);     // Centered: Move forward straight at 100% speed
  } else if (!s1 && s2 && !s3 && !s4 && !s5) {
    forward(100, 255);     // Soft drift Left: Left motor reduced, Right motor at MAX
  } else if (s1) {
    pivotLeft();           // Hard drift: Pivot Left on axis at MAX speed
  } else if (!s1 && !s2 && !s3 && s4 && !s5) {
    forward(255, 100);     // Soft drift Right: Left motor at MAX, Right motor reduced
  } else if (s5) {
    pivotRight();          // Hard drift: Pivot Right on axis at MAX speed
  } else {
    forward(255, 255);     // Track lost fallback: Search at MAX speed
  }

  delay(5); // Gives the Proteus engine breathing room to process calculations
}

// Moves forward with customizable max limits
void forward(int l, int r) {
  analogWrite(ENA, l);      analogWrite(ENB, r);
  digitalWrite(IN1, HIGH);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);
}

// Axis turn left at absolute 100% speed
void pivotLeft() {
  analogWrite(ENA, 255);    analogWrite(ENB, 255);
  digitalWrite(IN1, LOW);   digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);  digitalWrite(IN4, LOW);
}

// Axis turn right at absolute 100% speed
void pivotRight() {
  analogWrite(ENA, 255);    analogWrite(ENB, 255);
  digitalWrite(IN1, HIGH);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);   digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0);      analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);   digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);   digitalWrite(IN4, LOW);
}
