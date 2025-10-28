import cv2
import numpy as np
from ultralytics import YOLO
import time
import socket
import imutils
import threading
import queue
import torch

TCP_IP = '192.168.50.111'
TCP_PORT = 5005
url = 'http://192.168.50.111/stream'


def setup():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print(f"Connecting to ESP32 at {TCP_IP}:{TCP_PORT}...")
    s.connect((TCP_IP, TCP_PORT))
    print("Successfully connected to ESP32.")

    print("Loading YOLO model...")
    device = (
        "cuda" if torch.cuda.is_available()
        else "mps" if torch.backends.mps.is_available()
        else "cpu"
    )

    model = YOLO("yolo11n.pt")
    model.to(device)

    is_gpu = device in ("cuda", "mps")
    print("Successfully loaded YOLO model.")
    print("YOLO is running on", "GPU" if is_gpu else "CPU")

    return model, device


def stream_reader(frame_queue, url):
    print("Starting stream_reader thread...")
    vid = cv2.VideoCapture(url)
    i = 0
    time.sleep(2)

    while True:
        ret, frame = vid.read()
        if not ret:
            print("Couldn't read frame. Killing thread.")
            break

        if frame_queue.full():
            frame_queue.get()

        frame_queue.put(frame)
        i += 1


def stream_processor(frame_queue, model, device):
    print("Starting stream_processor thread...")
    while True:
        curr_frame = frame_queue.get()
        results = model(curr_frame, stream=True, device=device)

        for result in results:
            annotated_frame = result.plot()
            annotated_frame = imutils.resize(annotated_frame, width=600)
            cv2.imshow("YOLO Output", annotated_frame)

        if cv2.waitKey(1) == 27:  # ESC key
            break

    cv2.destroyAllWindows()


if __name__ == "__main__":
    frame_queue = queue.Queue(maxsize=2)
    model, device = setup()
    threading.Thread(target=stream_reader, args=(frame_queue, url), daemon=True).start()
    stream_processor(frame_queue, model, device)
