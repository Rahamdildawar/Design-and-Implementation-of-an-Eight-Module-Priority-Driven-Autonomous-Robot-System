import cv2
import mediapipe as mp
import serial

# --- Serial Setup ---
ser = serial.Serial('COM10', 9600)

# --- Camera Setup ---
cap = cv2.VideoCapture(0)

# --- MediaPipe Setup ---
mpHands = mp.solutions.hands
mpDraw = mp.solutions.drawing_utils
hands = mpHands.Hands(max_num_hands=1, min_detection_confidence=0.7)

# Tip landmark IDs for fingers (thumb to pinky)
tipIds = [4, 8, 12, 16, 20]

while True:
    success, img = cap.read()
    if not success:
        break

    img = cv2.flip(img, 1)  # Mirror image for natural feel
    rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    result = hands.process(rgb)

    fingers = 0

    if result.multi_hand_landmarks:
        for hand in result.multi_hand_landmarks:
            lm = hand.landmark

            # Thumb — compare X axis (left/right movement)
            if lm[tipIds[0]].x < lm[tipIds[0] - 1].x:
                fingers += 1

            # Four fingers — compare Y axis (up/down)
            for i in range(1, 5):
                if lm[tipIds[i]].y < lm[tipIds[i] - 2].y:
                    fingers += 1

            # Draw hand landmarks on frame
            mpDraw.draw_landmarks(img, hand, mpHands.HAND_CONNECTIONS)

    # --- Send Command to Arduino ---
    if fingers == 1:
        command = b'F'   # Forward
    elif fingers == 2:
        command = b'B'   # Backward
    elif fingers == 3:
        command = b'L'   # Left
    elif fingers == 4:
        command = b'R'   # Right
    else:
        command = b'S'   # Stop

    ser.write(command)

    # --- Display ---
    cv2.putText(img, f'Fingers: {fingers} | Cmd: {command.decode()}',
                (10, 40), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
    cv2.imshow("Hand Control", img)

    if cv2.waitKey(1) == 27:  # ESC to quit
        break

# --- Cleanup ---
cap.release()
ser.close()
cv2.destroyAllWindows()