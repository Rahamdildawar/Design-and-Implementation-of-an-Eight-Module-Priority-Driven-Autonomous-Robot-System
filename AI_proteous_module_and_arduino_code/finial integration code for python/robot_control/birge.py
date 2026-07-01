import cv2
import numpy as np
import serial
import time
import os
import json
import threading
import atexit
import logging
import urllib.request
from flask import Flask, request, jsonify, Response, render_template
from flask_cors import CORS

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s: %(message)s')

# ── Robust Tasks API MediaPipe Import ──────────────────────────
try:
    import mediapipe as mp
    from mediapipe.tasks import python
    from mediapipe.tasks.python import vision
    MEDIAPIPE = True
    logging.info("✔ MediaPipe Tasks API successfully imported")
except Exception as e:
    MEDIAPIPE = False
    logging.info(f"MediaPipe fallback active — gesture pipeline disabled. Error: {e}")

app = Flask(__name__, template_folder='templates')
CORS(app)

# ─── LOCKS ─────────────────────────────────────────────────────
state_lock = threading.Lock()
serial_lock = threading.Lock()

# ═══════════════════════════════════════════════════════════════
# SERIAL - WITH RECONNECT
# ═══════════════════════════════════════════════════════════════
SERIAL_PORT = os.getenv("SERIAL_PORT", "COM10")
BAUD_RATE = 9600
ser = None

def connect_serial():
    global ser
    try:
        if ser:
            ser.close()
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1, write_timeout=0.1)
        logging.info(f"✔ Serial connected: {SERIAL_PORT}")
    except Exception as e:
        logging.error(f"❌ Serial Connection Error: {e}")
        ser = None

connect_serial()

def tx(byte_char):
    """Thread-safe serial transmit with reconnect."""
    global ser
    if not ser:
        return
    try:
        with serial_lock:
            ser.write(byte_char.encode())
            logging.debug(f"TX → {byte_char}")
    except (serial.SerialTimeoutException, serial.SerialException) as e:
        logging.warning(f"Serial write error: {e} - attempting reconnect")
        connect_serial()
    except Exception as e:
        logging.error(f"TX transmission error: {e}")

# ═══════════════════════════════════════════════════════════════
# FACE RECOGNITION - MULTI PERSON LBPH
# ═══════════════════════════════════════════════════════════════
FACE_MODEL_PATH = "face_model.yml"
LABELS_PATH = "labels.json"
FACE_CONFIDENCE_THRESHOLD = 60

face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")
eye_cas = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_eye.xml")

def load_labels():
    if os.path.exists(LABELS_PATH):
        with open(LABELS_PATH, 'r') as f:
            return json.load(f)
    return {"name_to_id": {}, "id_to_name": {}, "next_id": 0}

def save_labels(data):
    with open(LABELS_PATH, 'w') as f:
        json.dump(data, f)

face_labels = load_labels()
recognizer = None

if os.path.exists(FACE_MODEL_PATH):
    try:
        if hasattr(cv2, 'face'):
            recognizer = cv2.face.LBPHFaceRecognizer_create()
            recognizer.read(FACE_MODEL_PATH)
            logging.info(f"✔ LBPH model loaded: {FACE_MODEL_PATH} | {len(face_labels['name_to_id'])} people")
        else:
            logging.error("❌ cv2.face module missing. Please run 'pip install opencv-contrib-python'")
            recognizer = None
    except Exception as e:
        logging.error(f"❌ LBPH model load error: {e}")
        recognizer = None
else:
    logging.warning(f"⚠ No face model at '{FACE_MODEL_PATH}' — face recognition defaults to STOP")

# ═══════════════════════════════════════════════════════════════
# MEDIAPIPE TASKS INITIALIZATION & MODEL AUTO-DOWNLOAD
# ═══════════════════════════════════════════════════════════════
MODEL_PATH = "hand_landmarker.task"
detector = None

if MEDIAPIPE:
    if not os.path.exists(MODEL_PATH):
        logging.info("⏳ Asset 'hand_landmarker.task' missing. Downloading from Google AI Edge repository...")
        try:
            url = "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
            urllib.request.urlretrieve(url, MODEL_PATH)
            logging.info("✔ Download complete.")
        except Exception as e:
            logging.error(f"❌ Asset auto-download failed: {e}")
            MEDIAPIPE = False

if MEDIAPIPE:
    try:
        base_options = python.BaseOptions(model_asset_path=MODEL_PATH)
        options = vision.HandLandmarkerOptions(
            base_options=base_options,
            num_hands=1,
            min_hand_detection_confidence=0.7,
            min_hand_presence_confidence=0.5,
            running_mode=vision.RunningMode.IMAGE
        )
        detector = vision.HandLandmarker.create_from_options(options)
        logging.info("✔ MediaPipe HandLandmarker Task engine active")
    except Exception as e:
        logging.error(f"❌ Failed to build Task engine: {e}")
        MEDIAPIPE = False

TIP_IDS = [4, 8, 12, 16, 20]
HAND_CONNECTIONS = [
    (0,1), (1,2), (2,3), (3,4),
    (0,5), (5,6), (6,7), (7,8),
    (5,9), (9,10), (10,11), (11,12),
    (9,13), (13,14), (14,15), (15,16),
    (13,17), (17,18), (18,19), (19,20), (0,17)
]

# ═══════════════════════════════════════════════════════════════
# STATE INTERRUPT INTERACTION ENGINE
# ═══════════════════════════════════════════════════════════════
state = {
    "scenario": "idle",
    "active_byte": "S",
    "pothole_debounce": 0,
    "face_db_count": len(face_labels["name_to_id"]),
    "face_count_live": 0,
    "finger_count": 0,
    "gesture_label": "—",
    "avoid_phase": "none",
    "avoid_timer": 0.0,
    "avoid_dir": "L",
    "face_label": "—",
    "face_confidence": 0,
    "fps": 0,
    "ph_min_area": 500,
    "ph_max_area": 25000,
    "ph_canny_lo": 30,
    "ph_canny_hi": 100,
    "ph_blur": 7,
    "ph_deb_thr": 3,
    "ph_asp_min": 20,
    "ph_asp_max": 450,
    "ph_solidity": 28,
    "ph_roi_top": 45,
}

# ═══════════════════════════════════════════════════════════════
# CAMERA INITIALIZATION - FIXED
# ═══════════════════════════════════════════════════════════════
def init_camera():
    """Initialize camera with fallbacks for Windows/Mac/Linux"""
    cam_idx = int(os.getenv("CAM_INDEX", 0))
    backends = []

    # Windows: DSHOW is most reliable, MSMF second
    if os.name == 'nt':
        backends = [cv2.CAP_DSHOW, cv2.CAP_MSMF, cv2.CAP_ANY]
    else:
        backends = [cv2.CAP_ANY, cv2.CAP_V4L2]

    for idx in [cam_idx, 0, 1, 2]: # Try requested index, then 0,1,2
        for backend in backends:
            try:
                cap = cv2.VideoCapture(idx, backend)
                if cap.isOpened():
                    # Test actual frame read
                    ret, _ = cap.read()
                    if ret:
                        cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
                        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
                        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
                        cap.set(cv2.CAP_PROP_FPS, 30)
                        cap.set(cv2.CAP_PROP_BUFFERSIZE, 1) # Reduce latency
                        logging.info(f"✔ Camera opened: index={idx}, backend={backend}")
                        return cap
                    cap.release()
            except Exception as e:
                logging.debug(f"Camera init failed idx={idx} backend={backend}: {e}")
                continue

    logging.error("❌ No camera found. Check if camera is connected and not used by other apps.")
    return None

camera = init_camera()
camera_lock = threading.Lock()

def cleanup():
    if camera and camera.isOpened():
        camera.release()
    if detector:
        try: detector.close()
        except: pass
    if ser and ser.is_open:
        tx("S")
        ser.close()
    logging.info("Cleanup run complete")

atexit.register(cleanup)

TURN_DUR = 1.2
CLEAR_DUR = 0.8

BYTE_COL = {
    "F": (60, 230, 60), "B": (60, 60, 230),
    "L": (230, 180, 0), "R": (0, 180, 230),
    "S": (160, 160, 160), "H": (0, 50, 230),
}

GESTURE_CMD = {
    1: ("F", "Index finger → FORWARD"),
    2: ("B", "2 fingers → BACKWARD"),
    3: ("L", "3 fingers → LEFT"),
    4: ("R", "4 fingers → RIGHT"),
    0: ("S", "Fist → STOP"),
}

def pothole_side(contours, frame_w):
    if not contours: return "L"
    biggest = max(contours, key=cv2.contourArea)
    bx, _, bw, _ = cv2.boundingRect(biggest)
    return "L" if (bx + bw // 2) > frame_w // 2 else "R"

def _state_snapshot(*keys):
    with state_lock: return {k: state[k] for k in keys}

def _state_set(**kwargs):
    with state_lock: state.update(kwargs)

# ═══════════════════════════════════════════════════════════════
# CORE GENERATIVE VISION MJPEG STREAMPIPE - CAMERA ERROR HANDLING
# ═══════════════════════════════════════════════════════════════
def stream():
    global camera
    prev_time = time.time()
    error_frame_count = 0

    while True:
        if camera is None or not camera.isOpened():
            # Camera failed - show error frame and try reconnect
            error_frame = np.zeros((480, 640, 3), dtype=np.uint8)
            cv2.putText(error_frame, "CAMERA NOT FOUND", (120, 220), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 0, 255), 3)
            cv2.putText(error_frame, "Check: 1) Camera connected 2) Not used by other app", (40, 260), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            cv2.putText(error_frame, "3) Try CAM_INDEX=0,1,2 env var", (140, 290), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
            ret, jpeg = cv2.imencode('.jpg', error_frame)
            yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n\r\n')

            # Try reconnect every 2 seconds
            time.sleep(2)
            with camera_lock:
                if camera:
                    camera.release()
                camera = init_camera()
            continue

        with camera_lock:
            ok, frame = camera.read()

        if not ok:
            error_frame_count += 1
            logging.warning(f"Camera read failed {error_frame_count} times")
            if error_frame_count > 10: # Reconnect after 10 failures
                with camera_lock:
                    camera.release()
                    camera = init_camera()
                error_frame_count = 0
            time.sleep(0.1)
            continue

        error_frame_count = 0
        frame = cv2.resize(frame, (640, 480))
        frame = cv2.flip(frame, 1)
        H, W = frame.shape[:2]

        now = time.time()
        fps = 1 / (now - prev_time + 1e-6)
        prev_time = now
        _state_set(fps=round(fps, 1))

        snap = _state_snapshot(
            "scenario", "active_byte", "ph_blur", "ph_canny_lo", "ph_canny_hi",
            "ph_min_area", "ph_max_area", "ph_deb_thr", "ph_asp_min", "ph_asp_max",
            "ph_solidity", "ph_roi_top", "pothole_debounce", "avoid_phase",
            "avoid_timer", "avoid_dir", "fps"
        )
        scenario = snap["scenario"]
        cur_byte = "S"

        # Global Embedded Realtime Telemetry HUD Layer
        cv2.rectangle(frame, (0, 0), (W, 28), (4, 4, 16), -1)
        cv2.putText(frame, f"RVCS v4.2 | {scenario.upper()} | {snap['fps']:.1f} FPS", (8, 18), cv2.FONT_HERSHEY_SIMPLEX, 0.45, (150, 200, 255), 1)

        # ─── MODE Pipeline 1: Pothole & Obstacle Extraction ───
        if scenario == "obstacle":
            blur_k = snap["ph_blur"] | 1
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            blur = cv2.GaussianBlur(gray, (blur_k, blur_k), 0)
            adapt = cv2.adaptiveThreshold(blur, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, 21, 4)
            edges = cv2.Canny(blur, snap["ph_canny_lo"], snap["ph_canny_hi"])
            mask = cv2.bitwise_or(edges, adapt)

            roi_y = int(H * snap["ph_roi_top"] / 100)
            roi = np.zeros_like(mask)
            cv2.fillPoly(roi, np.array([[(0,H),(0,roi_y),(W,roi_y),(W,H)]], np.int32), 255)
            mask = cv2.bitwise_and(mask, roi)
            cv2.line(frame, (0, roi_y), (W, roi_y), (255, 220, 0), 1)

            cnts, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            valid = []
            for c in cnts:
                area = cv2.contourArea(c)
                if snap["ph_min_area"] < area < snap["ph_max_area"]:
                    bx, by, bw, bh = cv2.boundingRect(c)
                    asp = bw / (bh + 1e-6)
                    hull_area = cv2.contourArea(cv2.convexHull(c))
                    sol = area / (hull_area + 1e-6)
                    if (snap["ph_asp_min"]/100.0) < asp < (snap["ph_asp_max"]/100.0) and sol > (snap["ph_solidity"]/100.0):
                        valid.append(c)
                        cv2.drawContours(frame, [c], -1, (0, 140, 255), 2)

            found = len(valid) > 0
            new_deb = min(10, snap["pothole_debounce"] + 2) if found else max(0, snap["pothole_debounce"] - 1)
            _state_set(pothole_debounce=new_deb)

            phase = snap["avoid_phase"]
            if phase == "none":
                cur_byte = pothole_side(valid, W) if (new_deb >= snap["ph_deb_thr"] and found) else "F"
                if cur_byte!= "F": _state_set(avoid_dir=cur_byte, avoid_phase="turning", avoid_timer=now)
            elif phase == "turning":
                cur_byte = snap["avoid_dir"]
                if now - snap["avoid_timer"] >= TURN_DUR: _state_set(avoid_phase="clear_check", avoid_timer=now)
            elif phase == "clear_check":
                cur_byte = "F"
                if now - snap["avoid_timer"] >= CLEAR_DUR: _state_set(avoid_phase="none", pothole_debounce=0)

            # Embedded Mini Monitoring Layout
            thumb = cv2.resize(cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR), (150,100))
            frame[330:430, 482:632] = thumb

        # ─── MODE Pipeline 2: Machine Learning Face Vector ───
        elif scenario == "face":
            gray = cv2.equalizeHist(cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY))
            faces = face_cascade.detectMultiScale(gray, 1.3, 5, minSize=(60,60))
            _state_set(face_count_live=len(faces))
            best_known, best_label, best_confidence = False, "No Face — STOP", 999

            for (fx, fy, fw, fh) in faces:
                face_crop = gray[fy:fy+fh, fx:fx+fw]
                if face_crop.size == 0: continue

                if recognizer is not None:
                    id_, confidence = recognizer.predict(cv2.resize(face_crop, (100,100)))
                    known = confidence < FACE_CONFIDENCE_THRESHOLD
                    name = face_labels["id_to_name"].get(str(id_), "Unknown")
                else: known, confidence, name = False, 999, "Unknown"

                if known: best_known = True
                if confidence < best_confidence:
                    best_confidence = confidence
                    best_label = f"{name} — FWD" if known else "Unknown — STOP"

                col = (0, 230, 80) if known else (0, 60, 230)
                cv2.rectangle(frame, (fx,fy), (fx+fw,fy+fh), col, 2)
                cv2.putText(frame, name, (fx, fy-8), cv2.FONT_HERSHEY_SIMPLEX, 0.5, col, 2)

            cur_byte = "F" if best_known else "S"
            _state_set(face_label=best_label, face_confidence=int(best_confidence) if len(faces) > 0 else 0)

        # ─── MODE Pipeline 3: MediaPipe Gesture Computation ───
        elif scenario == "gesture":
            fingers = 0
            if MEDIAPIPE and detector:
                rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                result = detector.detect(mp_image)
                if result.hand_landmarks and result.handedness:
                    for hand_idx, landmarks in enumerate(result.hand_landmarks):
                        is_ph_right = (result.handedness[hand_idx][0].category_name == "Left")
                        if is_ph_right:
                            if landmarks[TIP_IDS[0]].x < landmarks[TIP_IDS[0]-1].x: fingers += 1
                        else:
                            if landmarks[TIP_IDS[0]].x > landmarks[TIP_IDS[0]-1].x: fingers += 1
                        for i in range(1, 5):
                            if landmarks[TIP_IDS[i]].y < landmarks[TIP_IDS[i]-2].y: fingers += 1

                        pts = [ (int(lm.x * W), int(lm.y * H)) for lm in landmarks ]
                        for pt in pts: cv2.circle(frame, pt, 4, (0, 255, 250), -1)
                        for a, b in HAND_CONNECTIONS:
                            if a < len(pts) and b < len(pts): cv2.line(frame, pts[a], pts[b], (0, 200, 255), 1)

            _state_set(finger_count=fingers)
            cur_byte, gesture_lbl = GESTURE_CMD.get(fingers, ("S", "Fist → STOP"))
            _state_set(gesture_label=gesture_lbl)

        # ─── MODE Pipeline 4: Direct Remote Manual Drive ───
        elif scenario == "drive":
            cur_byte = snap["active_byte"]

        # ── Thread-Safe Serial Dispatch Line ──
        if cur_byte!= snap["active_byte"]:
            _state_set(active_byte=cur_byte)
            tx(cur_byte)

        ret, jpeg = cv2.imencode('.jpg', frame, [int(cv2.IMWRITE_JPEG_QUALITY), 85])
        if not ret: continue
        yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n\r\n')

# ═══════════════════════════════════════════════════════════════
# FLASK RESTFUL ROUTER CONTROL API BRIDGE
# ═══════════════════════════════════════════════════════════════
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/video_feed')
def video_feed():
    return Response(stream(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/api/state')
def get_state():
    with state_lock:
        return jsonify({
            "scenario": state["scenario"],
            "active_byte": state["active_byte"],
            "pothole_debounce": state["pothole_debounce"],
            "face_db_count": state["face_db_count"],
            "face_count_live": state["face_count_live"],
            "face_label": state["face_label"],
            "face_confidence": state["face_confidence"],
            "finger_count": state["finger_count"],
            "gesture_label": state["gesture_label"],
            "avoid_phase": state["avoid_phase"],
            "fps": state["fps"]
        })

@app.route('/api/command', methods=['POST'])
def cmd():
    b = request.json.get('byte','S')
    _state_set(active_byte=b)
    tx(b)
    return jsonify({"status":"ok"})

@app.route('/api/set_scenario', methods=['POST'])
def set_scenario():
    t = request.json.get('scenario','idle')
    with state_lock:
        state["scenario"] = t
        state["avoid_phase"] = "none"
        if t == 'idle':
            state["pothole_debounce"] = 0
            state["active_byte"] = "S"
    if t == 'idle': tx("S")
    return jsonify({"status":"ok","active":t})

if __name__ == '__main__':
    logging.info("🚀 Vibe Engine Operational On → http://127.0.0.1:5000")
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)