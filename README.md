# 🤖 Eight-Module Priority-Driven Autonomous Robot System

**Design and Implementation of an Eight-Module Priority-Driven Autonomous Robot System with Integrated Computer Vision and Multi-Sensor Fusion**

Author: **Raham Dil** — Computer Engineering, Ghulam Ishaq Khan Institute of Engineering Sciences and Technology (GIKI)
📧 u2024529@giki.edu.pk

<p align="center">
  <img src="./images/arduino_mega_2560.jpg" alt="Arduino Mega 2560" width="500"/>
</p>

<p align="center">
  <em>Platform: Arduino Mega 2560 + Python/OpenCV vision subsystem</em>
</p>

---

## 📖 Overview

This project presents a comprehensive **autonomous robot platform** built on the **Arduino Mega 2560**, augmented with a **laptop-based computer vision subsystem** written in **Python 3.9+ / OpenCV**. Eight independent hardware and software modules are unified under a **deterministic, five-level priority engine** that guarantees safe, predictable arbitration whenever multiple sensor events occur simultaneously (e.g., smoke detection + manual override + obstacle in path).

The system was validated experimentally, achieving **100% correct priority arbitration** across all tested concurrent-hazard scenarios.

---

## ✨ Key Contributions

- A **five-level priority engine** that deterministically arbitrates eight heterogeneous sensor-actuator modules in real time.
- An integrated **computer vision pipeline** (pothole detection, face dataset collection, gesture navigation zone) communicating with the embedded controller over a lightweight serial protocol.
- A **modular firmware/software architecture** enabling independent development and testing of each module while preserving system-wide safety invariants.
- **Empirical validation** of the priority engine under concurrent hazard conditions, with quantitative performance metrics.

---

## 🧩 System Modules

<table>
  <thead>
    <tr>
      <th align="center">#</th>
      <th align="left">Module</th>
      <th align="left">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center">1</td>
      <td><strong>Smoke &amp; Gas Detection</strong></td>
      <td>3× MQ-series gas sensors (L/C/R) — highest priority safety shutdown</td>
    </tr>
    <tr>
      <td align="center">2</td>
      <td><strong>Pothole Detection (CV)</strong></td>
      <td>OpenCV pipeline: Gaussian blur → Canny edges → contour/circularity filtering</td>
    </tr>
    <tr>
      <td align="center">3</td>
      <td><strong>Flame Tracking &amp; Suppression</strong></td>
      <td>3× KY-026 IR flame sensors, 2-stage track-then-suppress logic with fan actuator</td>
    </tr>
    <tr>
      <td align="center">4</td>
      <td><strong>Ultrasonic Obstacle Avoidance</strong></td>
      <td>3× HC-SR04 sensors, reverse–scan–turn maneuver below 18 cm</td>
    </tr>
    <tr>
      <td align="center">5</td>
      <td><strong>Face Dataset Collection</strong></td>
      <td>Viola–Jones (Haar cascade) face capture pipeline for downstream recognition training</td>
    </tr>
    <tr>
      <td align="center">6</td>
      <td><strong>Finger Gesture Control</strong></td>
      <td>ROI-based gesture navigation zone (ready for MediaPipe Hands integration)</td>
    </tr>
    <tr>
      <td align="center">7</td>
      <td><strong>Bluetooth Teleoperation</strong></td>
      <td>HC-05 module on <code>Serial1</code>, ASCII motion + speed protocol</td>
    </tr>
    <tr>
      <td align="center">8</td>
      <td><strong>Infrared Line Following</strong></td>
      <td>5× TCRT5000 sensors, rule-based sensor-to-action mapping (default behavior)</td>
    </tr>
  </tbody>
</table>

<p align="center">
  <img src="./images/ir_sensor_array.jpg" alt="5-Channel IR Sensor Array" width="320"/>
  <img src="./images/hcsr04_ultrasonic.jpg" alt="HC-SR04 Ultrasonic Sensor" width="320"/>
  <img src="./images/l298n_motor_driver.jpg" alt="L298N Motor Driver" width="320"/>
</p>
<p align="center"><em>Representative component images (5-ch IR array, HC-SR04, L298N driver) — not the project's own hardware photos.</em></p>

---

## 🏗️ System Architecture

The software stack is split into two tiers:

- **Embedded tier (C++/Arduino):** Runs at ~200 Hz on the Mega 2560. Implements the priority engine, all sensor drivers, and motor control primitives.
- **Vision tier (Python 3.9+):** Runs on a connected laptop at ~25–30 FPS. Implements Modules 2, 5, and 6 via OpenCV, sending single-byte ASCII decisions to the embedded tier over USB serial.

```
┌─────────────────────────┐        USB Serial (9600 baud)        ┌──────────────────────────┐
│   Python Vision Tier     │ ────────────────────────────────────▶│   Arduino Mega 2560       │
│  (OpenCV, 25–30 FPS)     │   ASCII tokens: P, C, F, B, L, R, S   │  Priority Engine (~200Hz) │
│  • Pothole Detection     │◀──────────────────────────────────── │  • 8 Modules              │
│  • Face Collector        │                                       │  • Motor Control (L298N)  │
│  • Gesture Zone          │                                       └──────────────┬───────────┘
└─────────────────────────┘                                                       │ Serial1 (9600 baud)
                                                                                    ▼
                                                                        ┌──────────────────────┐
                                                                        │   HC-05 Bluetooth     │
                                                                        │   (Teleoperation)     │
                                                                        └──────────────────────┘
```

### Priority Hierarchy

<table>
  <thead>
    <tr>
      <th align="center">Priority</th>
      <th align="left">Condition</th>
      <th align="center">Modules</th>
      <th align="left">Action</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td align="center"><strong>1 (highest)</strong></td>
      <td>Smoke OR pothole brake</td>
      <td align="center">1, 2</td>
      <td>Stop motors, buzzer ON, return</td>
    </tr>
    <tr>
      <td align="center"><strong>2</strong></td>
      <td>Flame detected</td>
      <td align="center">3</td>
      <td>Track &amp; suppress, return</td>
    </tr>
    <tr>
      <td align="center"><strong>3</strong></td>
      <td>Front obstacle &lt; 18 cm</td>
      <td align="center">4</td>
      <td>Reverse, scan, turn, return</td>
    </tr>
    <tr>
      <td align="center"><strong>4</strong></td>
      <td>Manual command within 700 ms</td>
      <td align="center">5, 6, 7</td>
      <td>Execute motion command, return</td>
    </tr>
    <tr>
      <td align="center"><strong>5 (default)</strong></td>
      <td>—</td>
      <td align="center">8</td>
      <td>Line following</td>
    </tr>
  </tbody>
</table>

Each level acts as a guard: if satisfied, all lower levels are bypassed via an early `return`, guaranteeing mutually exclusive behavior within a single control loop iteration.

---

## 🔌 Serial Communication Protocol

<table>
  <thead>
    <tr>
      <th align="center">Token</th>
      <th align="center">Direction</th>
      <th align="left">Meaning</th>
    </tr>
  </thead>
  <tbody>
    <tr><td align="center"><code>P</code></td><td align="center">PC → Mega</td><td>Pothole/hazard detected — halt</td></tr>
    <tr><td align="center"><code>C</code></td><td align="center">PC → Mega</td><td>Pothole cleared — resume</td></tr>
    <tr><td align="center"><code>F</code></td><td align="center">PC/BT → Mega</td><td>Move forward</td></tr>
    <tr><td align="center"><code>B</code></td><td align="center">PC/BT → Mega</td><td>Move backward</td></tr>
    <tr><td align="center"><code>L</code></td><td align="center">PC/BT → Mega</td><td>Pivot left</td></tr>
    <tr><td align="center"><code>R</code></td><td align="center">PC/BT → Mega</td><td>Pivot right</td></tr>
    <tr><td align="center"><code>S</code></td><td align="center">PC/BT → Mega</td><td>Stop motors</td></tr>
    <tr><td align="center"><code>0</code>–<code>9</code></td><td align="center">BT → Mega</td><td>Set speed (mapped 0–255)</td></tr>
  </tbody>
</table>

---

## 🛠️ Hardware / Pin Assignments (Arduino Mega 2560)

<table>
  <thead>
    <tr>
      <th align="left">Module</th>
      <th align="left">Signal</th>
      <th align="center">Pin</th>
      <th align="center">Direction</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>Motor Driver</td><td>ENA / IN1 / IN2 / IN3 / IN4 / ENB</td><td align="center">2 / 3 / 4 / 7 / 8 / 5</td><td align="center">OUT</td></tr>
    <tr><td>Module 8 (Line)</td><td>S1–S5</td><td align="center">22–26</td><td align="center">IN</td></tr>
    <tr><td>Module 3 (Flame)</td><td>FL, FC, FR</td><td align="center">30–32</td><td align="center">IN</td></tr>
    <tr><td>Module 1 (Smoke)</td><td>SL, SC, SR</td><td align="center">35–37</td><td align="center">IN</td></tr>
    <tr><td>Module 4 (Ultrasonic)</td><td>TRIG/ECHO × 3</td><td align="center">40–45</td><td align="center">OUT/IN</td></tr>
    <tr><td>Actuators</td><td>FAN / BUZZER</td><td align="center">9 / 10</td><td align="center">OUT</td></tr>
  </tbody>
</table>

> Full wiring schematics, PCB layout (2D & 3D), and simulation screenshots (flame/smoke, line follower, Bluetooth, full-integration) are available in [`/docs/circuit-diagrams`](./docs/circuit-diagrams) — see the original report's Figure 2 set.

---

## 💻 Software Structure

**Embedded Firmware (`/firmware`)**
```
firmware/
├── main.ino                  # setup(), main priority-engine loop
├── serial_handler.cpp        # readControlInputs()
├── ultrasonic.cpp            # getDistance()
├── line_follower.cpp         # runLineFollowingRoutine()
├── manual_dispatch.cpp       # executeManualAction()
└── motor_primitives.cpp      # forward(), backward(), pivotLeft/Right(), stopMotors()
```

**Vision System (`/vision`)**
```
vision/
├── config.py           # Config dataclass — tunable parameters
├── serial_manager.py   # SerialManager — pyserial wrapper w/ simulation fallback
├── obstacle_detector.py# ObstacleDetector — Module 2 (pothole detection)
├── face_collector.py   # FaceCollector — Module 5
├── gesture_zone.py     # GestureZone — Module 6 ROI overlay/accessor
├── hud.py              # HUD — on-screen status bar
└── main.py             # Entry point / frame processing loop
```

---

## 🚀 Getting Started

### Prerequisites
- Arduino IDE (or PlatformIO)
- Python 3.9+
- `opencv-python`, `pyserial` (`pip install opencv-python pyserial`)
- Arduino Mega 2560, L298N driver, HC-SR04 ×3, KY-026 ×3, MQ-series gas sensors ×3, TCRT5000 ×5, HC-05

### 1. Flash the firmware
```bash
# Open firmware/main.ino in Arduino IDE, select "Arduino Mega 2560", and upload
```

### 2. Run the vision system
```bash
cd vision
python main.py --port COM5 --baud 9600   # adjust port for your OS
```

### 3. Control the robot
- **Autonomous:** line following runs by default; hazards auto-preempt.
- **Manual (Bluetooth/keyboard):** send `F`, `B`, `L`, `R`, `S`, or a digit `0`–`9` for speed.

---

## 📊 Experimental Results

### Priority Engine Arbitration (20 trials/scenario)

<table>
  <thead>
    <tr>
      <th align="left">Scenario</th>
      <th align="center">Expected</th>
      <th align="center">Observed</th>
      <th align="center">Accuracy</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>Smoke only</td><td align="center">Stop+Buzz</td><td align="center">Stop+Buzz</td><td align="center">100%</td></tr>
    <tr><td>Flame only</td><td align="center">Track+Fan</td><td align="center">Track+Fan</td><td align="center">100%</td></tr>
    <tr><td>Obstacle only</td><td align="center">Avoid</td><td align="center">Avoid</td><td align="center">100%</td></tr>
    <tr><td>Smoke + Flame</td><td align="center">Stop+Buzz</td><td align="center">Stop+Buzz</td><td align="center">100%</td></tr>
    <tr><td>Smoke + Manual</td><td align="center">Stop+Buzz</td><td align="center">Stop+Buzz</td><td align="center">100%</td></tr>
    <tr><td>Flame + Manual</td><td align="center">Track+Fan</td><td align="center">Track+Fan</td><td align="center">100%</td></tr>
    <tr><td>Pothole (CV) + Manual</td><td align="center">Stop</td><td align="center">Stop</td><td align="center">100%</td></tr>
    <tr><td>All hazards active</td><td align="center">Stop+Buzz</td><td align="center">Stop+Buzz</td><td align="center">100%</td></tr>
  </tbody>
</table>

100% correct arbitration was observed across all 8 concurrent hazard scenarios.

### Pothole Detection (Module 2) — Confusion Matrix (200-frame test set)

<table>
  <thead>
    <tr>
      <th></th>
      <th align="center">Predicted: Pothole</th>
      <th align="center">Predicted: Clear</th>
      <th align="center">Total</th>
    </tr>
  </thead>
  <tbody>
    <tr><td><strong>Actual: Pothole</strong></td><td align="center">88</td><td align="center">12</td><td align="center">100</td></tr>
    <tr><td><strong>Actual: Clear</strong></td><td align="center">9</td><td align="center">91</td><td align="center">100</td></tr>
    <tr><td><strong>Total</strong></td><td align="center">97</td><td align="center">103</td><td align="center">200</td></tr>
  </tbody>
</table>

<table>
  <thead>
    <tr><th align="left">Metric</th><th align="center">Value</th></tr>
  </thead>
  <tbody>
    <tr><td>Precision</td><td align="center">90.7%</td></tr>
    <tr><td>Recall</td><td align="center">88.0%</td></tr>
    <tr><td>F1-Score</td><td align="center">0.894</td></tr>
  </tbody>
</table>

### Obstacle Avoidance (Module 4) — Threshold 18 cm

<table>
  <thead>
    <tr>
      <th align="center">Distance (cm)</th>
      <th align="center">Trials</th>
      <th align="center">Detected</th>
      <th align="center">Avoided</th>
      <th align="center">Accuracy</th>
    </tr>
  </thead>
  <tbody>
    <tr><td align="center">10</td><td align="center">10</td><td align="center">10</td><td align="center">10</td><td align="center">100%</td></tr>
    <tr><td align="center">15</td><td align="center">10</td><td align="center">10</td><td align="center">10</td><td align="center">100%</td></tr>
    <tr><td align="center">20</td><td align="center">10</td><td align="center">0</td><td align="center">0</td><td align="center">N/A</td></tr>
    <tr><td align="center">25</td><td align="center">10</td><td align="center">0</td><td align="center">0</td><td align="center">N/A</td></tr>
  </tbody>
</table>

### Line Following (Module 8)

<table>
  <thead>
    <tr>
      <th align="left">Track Type</th>
      <th align="center">Laps</th>
      <th align="center">Completed</th>
      <th align="center">Rate</th>
    </tr>
  </thead>
  <tbody>
    <tr><td>Straight</td><td align="center">10</td><td align="center">10</td><td align="center">100%</td></tr>
    <tr><td>Gentle curve (15°)</td><td align="center">10</td><td align="center">10</td><td align="center">100%</td></tr>
    <tr><td>Sharp curve (45°)</td><td align="center">10</td><td align="center">8</td><td align="center">80%</td></tr>
  </tbody>
</table>

### End-to-End Latency (Pothole Detection → Motor Stop, 50 trials)

<table>
  <thead>
    <tr><th align="left">Metric</th><th align="center">Value</th></tr>
  </thead>
  <tbody>
    <tr><td>Mean latency</td><td align="center">48.3 ms</td></tr>
    <tr><td>Standard deviation</td><td align="center">6.1 ms</td></tr>
    <tr><td>Maximum latency</td><td align="center">62 ms</td></tr>
  </tbody>
</table>

Dominant cost: USB-UART transmission at 9600 baud + camera frame period (~33 ms at 30 fps).

---

## ⚠️ Known Limitations

- **Blocking delays** in flame suppression (2 s) and obstacle avoidance (350/500 ms) pause serial processing — future work: interrupt/timer-driven non-blocking control.
- **Pothole detector** (classical CV) is sensitive to lighting; a lightweight CNN (e.g., MobileNetV2) is recommended for outdoor robustness.
- **Gesture recognition (Module 6)** currently only renders the ROI — classifier (e.g., MediaPipe Hands) not yet implemented.
- **Serial protocol** has no checksum/error correction; a lightweight XOR checksum is a suggested improvement.

---

## 🔭 Future Work

- Replace blocking delays with timer-interrupt–based non-blocking suppression/avoidance routines.
- Integrate a CNN-based pothole detector.
- Implement the Module 6 gesture classifier (MediaPipe Hands or custom CNN).
- Add a lightweight serial error-detection/checksum scheme.
- Increase baud rate to 115200 to reduce end-to-end latency.

---

## 📚 References

Key references include Brooks' subsumption architecture, Arkin's motor-schema navigation, Siegwart & Nourbakhsh's *Introduction to Autonomous Mobile Robots*, Viola & Jones' cascade face detector, and ROS (Quigley et al.). See the full report for the complete bibliography.

---

## 🙏 Acknowledgment

Thanks to the Department of Electrical Engineering (GIKI) for providing laboratory facilities and hardware components used in this project.

---

## 📄 License

This project is released for educational and research purposes. Add your preferred license (e.g., MIT) here.
