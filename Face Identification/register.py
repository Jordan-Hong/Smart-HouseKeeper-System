# ==========================================
# register.py
# 建立並儲存人臉資料庫
# ==========================================

import os
import cv2
import pickle
import numpy as np

from mtcnn import MTCNN
from keras_facenet import FaceNet

# ----------------------------
# 初始化
# ----------------------------

detector = MTCNN()
embedder = FaceNet()

DATASET_PATH = "dataset"

database = {}

# ----------------------------
# 取得 embedding
# ----------------------------

def get_embedding(image_path):

    img = cv2.imread(image_path)

    if img is None:
        print(f"無法讀取: {image_path}")
        return None

    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

    results = detector.detect_faces(img_rgb)

    if len(results) == 0:
        print(f"找不到人臉: {image_path}")
        return None

    x, y, w, h = results[0]['box']

    x, y = abs(x), abs(y)

    face = img_rgb[y:y+h, x:x+w]

    face = cv2.resize(face, (160, 160))

    face = np.expand_dims(face, axis=0)

    embedding = embedder.embeddings(face)

    return embedding[0]

# ----------------------------
# 建立 database
# ----------------------------

for person_name in os.listdir(DATASET_PATH):

    person_folder = os.path.join(DATASET_PATH, person_name)

    if not os.path.isdir(person_folder):
        continue

    embeddings = []

    for filename in os.listdir(person_folder):

        image_path = os.path.join(person_folder, filename)
        print(image_path)
        embedding = get_embedding(image_path)

        if embedding is not None:
            embeddings.append(embedding)

    if len(embeddings) > 0:

        mean_embedding = np.mean(embeddings, axis=0)

        database[person_name] = mean_embedding

        print(f"{person_name} 已加入資料庫")

# ----------------------------
# 儲存 database
# ----------------------------

with open("face_database.pkl", "wb") as f:
    pickle.dump(database, f)

print("\nDatabase 已儲存")
print(database.keys())