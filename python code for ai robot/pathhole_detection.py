import cv2
import serial
import time
import numpy as np

# ── Serial setup ─────────────────────────────────────────────────────
try:
    ser = serial.Serial('COM10', 9600, timeout=1)
    time.sleep(2)
    print("Serial connected on COM10")
except serial.SerialException as e:
    print(f"Serial error: {e}")
    ser = None

if ser is None:
    print("WARNING: No serial connection — motors will not run!")

# ── Auto-detect camera ───────────────────────────────────────────────
def find_camera():
    for index in range(5):
        cap = cv2.VideoCapture(index, cv2.CAP_DSHOW)
        if cap.isOpened():
            ret, frame = cap.read()
            if ret and frame is not None:
                print(f"Camera found at index {index}")
                return cap
            cap.release()
    return None

cap = find_camera()
if cap is None:
    print("No camera found!")
    exit()

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
cap.set(cv2.CAP_PROP_FPS, 30)

# ── Trackbar window ──────────────────────────────────────────────────
cv2.namedWindow("Settings", cv2.WINDOW_NORMAL)
cv2.resizeWindow("Settings", 500, 400)

cv2.createTrackbar("Min Area /10",  "Settings", 300,  2000, lambda x: None)
cv2.createTrackbar("Blur Kernel",   "Settings", 7,    21,   lambda x: None)
cv2.createTrackbar("Block Size",    "Settings", 31,   99,   lambda x: None)
cv2.createTrackbar("C Value",       "Settings", 10,   30,   lambda x: None)
cv2.createTrackbar("ROI Top %",     "Settings", 40,   90,   lambda x: None)
cv2.createTrackbar("Safety Margin", "Settings", 60,   200,  lambda x: None)
cv2.createTrackbar("Threshold Mode","Settings", 0,    1,    lambda x: None)
cv2.createTrackbar("Simple Thresh", "Settings", 60,   255,  lambda x: None)
cv2.createTrackbar("Morph Iter",    "Settings", 2,    5,    lambda x: None)

# ── Command sending ──────────────────────────────────────────────────
last_command = None
command_counter = 0
RESEND_EVERY = 10  # resend same command every 10 frames to keep motor alive

def send_command(cmd: bytes):
    global last_command, command_counter
    if ser:
        try:
            command_counter += 1
            if cmd != last_command or command_counter >= RESEND_EVERY:
                ser.write(cmd)
                last_command = cmd
                command_counter = 0
                print(f"CMD: {cmd.decode()}")
        except Exception as e:
            print(f"Serial send error: {e}")
            last_command = None  # reset so it retries next frame
    else:
        print("WARNING: Serial not connected — no command sent!")

# ── Trackbar reader ──────────────────────────────────────────────────
def get_trackbar_values():
    min_area   = cv2.getTrackbarPos("Min Area /10",  "Settings") * 10
    blur       = cv2.getTrackbarPos("Blur Kernel",   "Settings")
    block      = cv2.getTrackbarPos("Block Size",    "Settings")
    c_val      = cv2.getTrackbarPos("C Value",       "Settings")
    roi_pct    = cv2.getTrackbarPos("ROI Top %",     "Settings")
    safety     = cv2.getTrackbarPos("Safety Margin", "Settings")
    mode       = cv2.getTrackbarPos("Threshold Mode","Settings")
    simple_t   = cv2.getTrackbarPos("Simple Thresh", "Settings")
    morph_iter = cv2.getTrackbarPos("Morph Iter",    "Settings")

    if blur % 2 == 0:   blur += 1
    if block % 2 == 0:  block += 1
    if block < 3:       block = 3
    if min_area < 100:  min_area = 100

    return min_area, blur, block, c_val, roi_pct, safety, mode, simple_t, morph_iter

# ── Preprocessing ────────────────────────────────────────────────────
def preprocess(frame, blur, block, c_val, mode, simple_t, morph_iter):
    gray    = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    blurred = cv2.GaussianBlur(gray, (blur, blur), 0)

    if mode == 0:
        thresh = cv2.adaptiveThreshold(
            blurred, 255,
            cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
            cv2.THRESH_BINARY_INV,
            blockSize=block, C=c_val
        )
    else:
        _, thresh = cv2.threshold(blurred, simple_t, 255, cv2.THRESH_BINARY_INV)

    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    thresh = cv2.morphologyEx(thresh, cv2.MORPH_OPEN,  kernel, iterations=morph_iter)
    thresh = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, kernel, iterations=morph_iter)
    return thresh

# ── Hole danger logic ────────────────────────────────────────────────
def get_hole_danger(contours, frame_w, frame_h, min_area, roi_pct, safety):
    danger_rects   = []
    left_danger    = False
    right_danger   = False
    center_blocked = False
    third          = frame_w // 3
    roi_y_start    = int(frame_h * roi_pct / 100)

    for c in contours:
        area = cv2.contourArea(c)
        if area < min_area:
            continue

        x, y, w, h = cv2.boundingRect(c)
        if y + h < roi_y_start:
            continue

        aspect = w / float(h) if h > 0 else 999
        if aspect > 8 or aspect < 0.1:
            continue

        danger_rects.append((x, y, w, h))
        center_x   = x + w // 2
        left_edge  = x
        right_edge = x + w

        if left_edge < (frame_w // 2 + safety) and \
           right_edge > (frame_w // 2 - safety):
            center_blocked = True

        if center_x < third:
            left_danger = True
        elif center_x > 2 * third:
            right_danger = True
        else:
            center_blocked = True

    if not danger_rects:
        return 'F', []
    if center_blocked:
        if left_danger and not right_danger:
            return 'R', danger_rects
        elif right_danger and not left_danger:
            return 'L', danger_rects
        else:
            return 'S', danger_rects
    if left_danger:
        return 'R', danger_rects
    if right_danger:
        return 'L', danger_rects
    return 'F', danger_rects

# ── Display config ───────────────────────────────────────────────────
COLORS = {
    'F': (0, 255,   0),
    'L': (0, 165, 255),
    'R': (255, 165,  0),
    'S': (0,   0, 255),
}
LABELS = {'F': 'FORWARD', 'L': 'TURN LEFT', 'R': 'TURN RIGHT', 'S': 'STOP'}

consecutive_fails = 0

print("\n=== TUNING GUIDE ===")
print("Too many false holes  → increase 'Min Area' or 'C Value'")
print("Missing real holes    → decrease 'Min Area' or 'C Value'")
print("Noisy edges           → increase 'Blur Kernel' or 'Morph Iter'")
print("Bad lighting          → switch 'Threshold Mode' 0↔1")
print("Detecting background  → increase 'ROI Top %'")
print("Press ESC to quit\n")

# ── Main loop ────────────────────────────────────────────────────────
while True:
    ret, frame = cap.read()

    if not ret or frame is None:
        consecutive_fails += 1
        if consecutive_fails >= 10:
            print("Camera lost. Exiting.")
            break
        time.sleep(0.05)
        continue

    consecutive_fails = 0
    h, w = frame.shape[:2]

    min_area, blur, block, c_val, roi_pct, safety, mode, simple_t, morph_iter = get_trackbar_values()

    thresh = preprocess(frame, blur, block, c_val, mode, simple_t, morph_iter)
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    decision, holes = get_hole_danger(contours, w, h, min_area, roi_pct, safety)

    # ── DEBUG LINE — watch this in terminal ──────────────────────────
    print(f"Decision={decision}  Holes={len(holes)}  Serial={'OK' if ser else 'NONE'}")

    send_command(decision.encode())

    # ── Draw overlay ─────────────────────────────────────────────────
    roi_y = int(h * roi_pct / 100)
    overlay = frame.copy()

    cv2.rectangle(overlay, (0, 0), (w, roi_y), (60, 20, 20), -1)
    cv2.addWeighted(overlay, 0.3, frame, 0.7, 0, frame)

    cv2.line(frame, (0, roi_y),     (w, roi_y),     (0, 220, 220), 2)
    cv2.line(frame, (w//3, roi_y),  (w//3, h),      (180, 180, 180), 1)
    cv2.line(frame, (2*w//3, roi_y),(2*w//3, h),    (180, 180, 180), 1)

    cv2.putText(frame, "LEFT",   (10,        h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180,180,180), 1)
    cv2.putText(frame, "CENTER", (w//3 + 10, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180,180,180), 1)
    cv2.putText(frame, "RIGHT",  (2*w//3+10, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180,180,180), 1)

    for (x, y, rw, rh) in holes:
        area_val = rw * rh
        cv2.rectangle(frame, (x, y), (x+rw, y+rh), (0, 0, 255), 2)
        cv2.putText(frame, f"HOLE {area_val}px", (x, max(y-8, 15)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

    color = COLORS[decision]
    cv2.rectangle(frame, (0, 0), (w, 50), (20, 20, 20), -1)
    cv2.putText(frame, f"Action: {LABELS[decision]}",
                (10, 32), cv2.FONT_HERSHEY_SIMPLEX, 1.0, color, 2)
    cv2.putText(frame, f"Holes: {len(holes)}  Area>={min_area}",
                (w - 260, 32), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (200,200,200), 1)

    mode_text = "ADAPTIVE" if mode == 0 else f"SIMPLE T={simple_t}"
    cv2.putText(frame, mode_text, (w - 180, h - 10),
                cv2.FONT_HERSHEY_SIMPLEX, 0.45, (0, 220, 220), 1)

    cv2.imshow("Pothole Detection", frame)
    cv2.imshow("Threshold View",    thresh)

    if cv2.waitKey(1) == 27:
        break

# ── Cleanup ───────────────────────────────────────────────────────────
send_command(b'S')
cap.release()
cv2.destroyAllWindows()
if ser:
    ser.close()