import cv2
import pickle
import numpy as np

from mtcnn import MTCNN
from keras_facenet import FaceNet
from sklearn.metrics.pairwise import cosine_similarity
from state import system_state

detector = MTCNN()
embedder = FaceNet()

THRESHOLD = 0.7

with open("face_database.pkl", "rb") as f:
    database = pickle.load(f)

frame_count = 0
last_results = []

def recognize_face(frame):

    global frame_count
    global last_results

    frame_count += 1

    if frame_count % 10 == 0:

        img_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        results = detector.detect_faces(img_rgb)

        temp_results = []

        for result in results:

            x, y, w, h = result['box']

            x, y = abs(x), abs(y)

            face = img_rgb[y:y+h, x:x+w]

            try:
                face = cv2.resize(face, (160, 160))
            except:
                continue

            face_input = np.expand_dims(face, axis=0)

            embedding = embedder.embeddings(face_input)[0]

            best_match = "Unknown"
            best_score = -1

            for person_name, db_embedding in database.items():

                similarity = cosine_similarity(
                    [embedding],
                    [db_embedding]
                )[0][0]

                if similarity > best_score:
                    best_score = similarity
                    best_match = person_name

            if best_score < THRESHOLD:
                best_match = "Unknown"
            
            # 更新全域狀態
            system_state["person"] = best_match

            temp_results.append({
                "box": (x, y, w, h),
                "name": best_match,
                "score": best_score
            })

        last_results = temp_results

    return last_results