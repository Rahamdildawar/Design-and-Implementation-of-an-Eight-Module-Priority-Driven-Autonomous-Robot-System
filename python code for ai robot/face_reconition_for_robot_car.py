import cv2
import pickle
import serial
import numpy as np

# --- Serial Setup ---
ser = serial.Serial('COM11', 9600)

# --- Load trained face model ---
recognizer = cv2.face.LBPHFaceRecognizer_create()
recognizer.read(r"c:\Users\Engr.Raham_Dil\Desktop\python code for ai robot\face_model.yml")

# --- Setup face detector ---
detector = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")

# --- Camera Setup ---
cap = cv2.VideoCapture(0)
print("✅ Face Recognition Robot Started!")
print("📷 Show your face to move the robot!")
print("⌨️  Press ESC to quit")

while True:
    success, img = cap.read()
    if not success:
        break

    img = cv2.flip(img, 1)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    faces = detector.detectMultiScale(gray, 1.3, 5)

    command = b'S'
    label = "No Face - STOP"
    color = (0, 0, 255)

    for (x, y, w, h) in faces:
        face_crop = gray[y:y+h, x:x+w]
        face_resized = cv2.resize(face_crop, (100, 100))

        id_, confidence = recognizer.predict(face_resized)

        # Adjust confidence threshold if needed (50-80 range)
        if confidence < 80:
            command = b'F'
            label = f"Known Face - FORWARD ({int(confidence)})"
            color = (0, 255, 0)
        else:
            command = b'S'
            label = f"Unknown - STOP ({int(confidence)})"
            color = (0, 0, 255)

        cv2.rectangle(img, (x, y), (x+w, y+h), color, 2)
        cv2.putText(img, label, (x, y-10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, color, 2)

    ser.write(command)

    cv2.imshow("Face Recognition Robot", img)

    if cv2.waitKey(1) == 27:
        break

cap.release()
ser.close()
cv2.destroyAllWindows()