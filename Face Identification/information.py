import time
import cv2
import numpy as np
from state import system_state

def create_info_panel(frame):

    if not hasattr(create_info_panel, "last_greeting"):
        create_info_panel.last_greeting = "N/A"
        create_info_panel.last_time = 0

    info_panel_width = 400

    height, width, _ = frame.shape

    info_panel = np.zeros(
        (height, info_panel_width, 3),
        dtype=np.uint8
    )

    person = system_state["person"]
    counter = system_state["counter"]
    front = system_state["frontValue"]
    rear = system_state["rearValue"]
    greeting = system_state["greeting"]

    if greeting != create_info_panel.last_greeting:

        create_info_panel.last_greeting = greeting
        create_info_panel.last_time = time.time()

    cv2.putText(
        info_panel,
        f"Back-end",
        (20, 60),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255,255,255),
        2
    )

    cv2.putText(
        info_panel,
        f"Last : {person}",
        (20, 120),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255,255,255),
        2
    )

    cv2.putText(
        info_panel,
        f"counter : {counter}",
        (20, 180),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255,255,255),
        2
    )

    cv2.putText(
        info_panel,
        f"frontValue : {front}",
        (20, 240),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255,255,255),
        2
    )

    cv2.putText(
        info_panel,
        f"rearValue : {rear}",
        (20, 300),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255,255,255),
        2
    )
    
    cv2.putText(
        info_panel,
        f"Greeting: {greeting}",
        (20, 360),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.8,
        (255,255,255),
        2
    )

    # frame
    show_greeting = False

    if greeting != "N/A":

        if time.time() - create_info_panel.last_time < 3:
            show_greeting = True
        else:
            system_state["greeting"] = "N/A"

    if show_greeting:

        overlay = frame.copy()

        cv2.rectangle(
            overlay,
            (20,20),
            (320,90),
            (40,40,40),
            -1
        )

        cv2.addWeighted(
            overlay,
            0.6,
            frame,
            0.4,
            0,
            frame
        )

        cv2.putText(
            frame,
            greeting,
            (40,65),
            cv2.FONT_HERSHEY_SIMPLEX,
            1,
            (255,255,255),
            2
        )

    # combined = np.hstack((frame, info_panel))

    return frame, info_panel