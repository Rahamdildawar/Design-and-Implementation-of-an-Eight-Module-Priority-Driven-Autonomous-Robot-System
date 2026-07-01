import cv2
import pickle

# Full path to your image
img = cv2.imread(r"WIN_20260605_16_11_00_Pro.jpg")

if img is None:
    print("ERROR: Image not found! Check the path.") 
else:
    detector = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    faces = detector.detectMultiScale(gray, 1.3, 5)

    if len(faces) > 0:
        x, y, w, h = faces[0]
        face_crop = gray[y:y+h, x:x+w]
        face_resized = cv2.resize(face_crop, (100, 100))
        pickle.dump(face_resized, open(r"c:\Users\Engr.Raham_Dil\Desktop\python code for ai robot\face.dat", "wb"))
        print("Face encoded and saved successfully!")
    else:
        print("No face detected in image!")