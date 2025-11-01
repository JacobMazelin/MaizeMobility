import cv2
import numpy as np
from ultralytics import YOLO
import time
import socket
import imutils
import threading
import queue
import torch

#TODO Change port to upper camera
TCP_IP = '192.168.50.111'
TCP_PORT = 5005
url = 'http://192.168.50.111/stream'

# COCO class ID for stop sign
STOP_SIGN_CLASS_ID = 11

# Calibration constants (you'll need to tune these)
# Real-world size of a stop sign in inches (typical US stop sign is 30 inches)
REAL_STOP_SIGN_HEIGHT_INCHES = 30.0

# Focal length estimate (in pixels) - calculate from your camera or estimate
# Formula: focal_length = (detected_height_pixels * known_distance) / real_object_height
# Example: If at 5 feet (60 inches), stop sign appears 100 pixels tall:
# FOCAL_LENGTH = (100 * 60) / 30 = 200 pixels
FOCAL_LENGTH = 200.0  # Start with estimate, tune based on testing

def calculate_distance(box_height_pixels, real_height_inches, focal_length):
    """
    Calculate distance using similar triangles principle.
    
    Distance = (real_height * focal_length) / detected_height
    
    Args:
        box_height_pixels: Height of bounding box in pixels
        real_height_inches: Real-world height of object in inches
        focal_length: Camera focal length in pixels
    
    Returns:
        Distance in inches
    """
    if box_height_pixels == 0:
        return None
    distance = (real_height_inches * focal_length) / box_height_pixels
    return distance

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

    model = YOLO("/Users/jacobmazelin/Main-Desktop/All_Code/UM/Classes/ENGR100/project_environment/Code/line_follow/line_follow_commands/yolo11n.pt")  
    model.to(device)

    is_gpu = device in ("cuda", "mps")
    print("Successfully loaded YOLO model.")
    print("YOLO is running on", "GPU" if is_gpu else "CPU")

    return model, device, s

def stream_reader(frame_queue, url):
    print("Starting stream_reader thread...")
    vid = cv2.VideoCapture(url)
    time.sleep(2)

    while True:
        ret, frame = vid.read()
        if not ret:
            print("Couldn't read frame. Killing thread.")
            break

        if frame_queue.full():
            frame_queue.get()

        frame_queue.put(frame)

def stream_processor(frame_queue, model, device, sock):
    print("Starting stream_processor thread...")
    while True:
        if frame_queue.empty():
            time.sleep(0.01)
            continue
            
        curr_frame = frame_queue.get()
        results = model(curr_frame, stream=True, device=device, verbose=False)

        for result in results:
            # Get all detections
            boxes = result.boxes
            
            # Filter for stop signs (class ID 11)
            stop_signs = []
            for box in boxes:
                class_id = int(box.cls[0])
                confidence = float(box.conf[0])
                
                if class_id == STOP_SIGN_CLASS_ID and confidence > 0.5:  # Confidence threshold
                    # Get bounding box coordinates
                    x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                    x1, y1, x2, y2 = int(x1), int(y1), int(x2), int(y2)
                    
                    # Calculate bounding box dimensions
                    box_width = x2 - x1
                    box_height = y2 - y1
                    box_center_x = (x1 + x2) // 2
                    box_center_y = (y1 + y2) // 2
                    
                    # Estimate distance
                    distance = calculate_distance(box_height, REAL_STOP_SIGN_HEIGHT_INCHES, FOCAL_LENGTH)
                    
                    stop_signs.append({
                        'bbox': (x1, y1, x2, y2),
                        'confidence': confidence,
                        'distance': distance,
                        'center': (box_center_x, box_center_y),
                        'size': (box_width, box_height)
                    })
            
            # Draw detections and info on frame
            annotated_frame = curr_frame.copy()
            
            if stop_signs:
                # Sort by distance (closest first)
                stop_signs.sort(key=lambda x: x['distance'] if x['distance'] else float('inf'))
                closest = stop_signs[0]
                
                x1, y1, x2, y2 = closest['bbox']
                
                # Draw bounding box
                cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
                
                # Draw distance info
                distance_text = f"Stop Sign: {closest['distance']:.1f} in" if closest['distance'] else "Stop Sign: Unknown dist"
                conf_text = f"Conf: {closest['confidence']:.2f}"
                
                cv2.putText(annotated_frame, distance_text, (x1, y1 - 20), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
                cv2.putText(annotated_frame, conf_text, (x1, y1 - 5), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)
                
                # Draw center point
                cv2.circle(annotated_frame, closest['center'], 5, (255, 0, 0), -1)
                
                # Send command to Arduino/ESP32 if close enough
                if closest['distance'] and closest['distance'] < 60:  # Less than 5 feet
                    print(f"STOP SIGN DETECTED at {closest['distance']:.1f} inches!")
                    # Send STOP command (adjust based on your protocol)
                    try:
                        sock.sendall(b"STOP\n")
                    except:
                        pass
                
                # Print all detected stop signs
                print(f"Found {len(stop_signs)} stop sign(s):")
                for i, ss in enumerate(stop_signs):
                    dist_str = f"{ss['distance']:.1f} in" if ss['distance'] else "Unknown"
                    print(f"  #{i+1}: {dist_str}, Conf: {ss['confidence']:.2f}")
            else:
                cv2.putText(annotated_frame, "No Stop Sign", (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            
            # Resize and display
            annotated_frame = imutils.resize(annotated_frame, width=600)
            cv2.imshow("Stop Sign Detection", annotated_frame)

        if cv2.waitKey(1) == 27:  # ESC key
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    frame_queue = queue.Queue(maxsize=2)
    model, device, sock = setup()
    threading.Thread(target=stream_reader, args=(frame_queue, url), daemon=True).start()
    stream_processor(frame_queue, model, device, sock)