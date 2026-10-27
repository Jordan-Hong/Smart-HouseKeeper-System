import time
import serial
import threading
from state import system_state

ser = serial.Serial('COM6', 9600, timeout=1)

recognition_result = None

recognition_mode = False
recognition_deadline = 0


def set_recognition_result(result):
    global recognition_result
    recognition_result = result


def serial_listener():

    global recognition_mode
    global recognition_deadline
    global recognition_result

    while True:

        # ==========================
        # 檢查辨識是否完成
        # ==========================

        if recognition_mode:

            # 成功辨識
            if recognition_result is True:

                ser.write(b"True\n")

                recognition_mode = False
                recognition_result = None
                system_state["greeting"] = "Welcome back!"
                print("Send: True")

            # 等待超時
            elif time.time() > recognition_deadline:

                ser.write(b"False\n")

                recognition_mode = False
                recognition_result = None

                print("Send: False (Timeout)")

        # ==========================
        # 接收 Arduino 訊息
        # ==========================

        if ser.in_waiting:

            msg = ser.readline().decode().strip()

            print(msg)

            if msg == "recognition_request":

                print("Start Recognition Window")

                recognition_result = None

                recognition_mode = True

                recognition_deadline = time.time() + 10

            elif msg.startswith("counter="):
                
                prev_counter = system_state["counter"]
                system_state["counter"] = int(
                    msg.split("=")[1]
                )
                if (system_state["counter"]>prev_counter):
                    pass
                    # system_state["greeting"] = "Welcome back!"
                elif (system_state["counter"]<prev_counter):
                    system_state["greeting"] = "Good day!"

            elif msg.startswith("frontValue="):

                system_state["frontValue"] = int(
                    msg.split("=")[1]
                )

            elif msg.startswith("rearValue="):

                system_state["rearValue"] = int(
                    msg.split("=")[1]
                )

            elif msg.startswith("rearValue="):

                system_state["rearValue"] = int(
                    msg.split("=")[1]
                )

            elif msg == "greeting=A":
                system_state["greeting"] = "Good day!"
            elif msg == "greeting=B":
                system_state["greeting"] = "Power saved."
        time.sleep(0.01)


def start():

    threading.Thread(
        target=serial_listener,
        daemon=True
    ).start()