import cv2
import pickle
import numpy as np

detector = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")
recognizer = cv2.face.LBPHFaceRecognizer_create()

cap = cv2.VideoCapture(0)
faces_data = []
count = 0

print("📷 Look at camera... collecting 30 face samples")

while True:
    success, img = cap.read()
    if not success:
        break

    img = cv2.flip(img, 1)
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    faces = detector.detectMultiScale(gray, 1.3, 5)

    for (x, y, w, h) in faces:
        face_crop = gray[y:y+h, x:x+w]
        face_resized = cv2.resize(face_crop, (100, 100))

        if count < 30:
            faces_data.append(face_resized)
            count += 1
            cv2.rectangle(img, (x, y), (x+w, y+h), (0, 255, 0), 2)
            cv2.putText(img, f"Capturing... {count}/30",
                        (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

    cv2.imshow("Collecting Face Data - Press ESC when done", img)

    if count >= 30:
        print("✅ 30 samples collected!")
        break

    if cv2.waitKey(1) == 27:
        break

cap.release()
cv2.destroyAllWindows()

# Train recognizer with 30 samples
if len(faces_data) > 0:
    labels = np.array([1] * len(faces_data))
    recognizer.train(faces_data, labels)
    
    # Save both raw faces and trained model
    pickle.dump(faces_data, open(r"c:\Users\Engr.Raham_Dil\Desktop\python code for ai robot\face.dat", "wb"))
    recognizer.save(r"c:\Users\Engr.Raham_Dil\Desktop\python code for ai robot\face_model.yml")
    print("✅ Face model saved successfully!")
else:
    print("❌ No face detected! Try better lighting.")