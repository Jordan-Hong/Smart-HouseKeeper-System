import cv2

from recognize import recognize_face
from information import create_info_panel
from state import system_state
from Agent_1 import start, set_recognition_result

start()
cap = cv2.VideoCapture(0)

cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

while True:

    ret, frame = cap.read()

    if not ret:
        break

    # -------------------------
    # 人臉辨識
    # -------------------------
    
    results = recognize_face(frame)
    recognized = None

    if len(results) > 0:

        recognized = False

        for r in results:

            if r["name"] != "Unknown":
                recognized = True
                break
    
    set_recognition_result(recognized)

    # 畫框
    for r in results:

        x, y, w, h = r["box"]

        name = r["name"]

        score = r["score"]

        cv2.rectangle(
            frame,
            (x, y),
            (x+w, y+h),
            (0,255,0),
            2
        )

        text = f"{name} ({score:.2f})"

        cv2.putText(
            frame,
            text,
            (x, y-10),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0,255,0),
            2
        )

    # -------------------------
    # Information Module
    # -------------------------

    frame, info_panel = create_info_panel(frame)

    # -------------------------
    # 顯示
    # -------------------------

    cv2.imshow("Front-End", frame)
    cv2.imshow("Back-End", info_panel)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()