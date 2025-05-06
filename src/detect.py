import cv2
import urllib.request
import numpy as np
import time
import requests
import os
from ultralytics import YOLO

# Load model
model = YOLO("fire_detection_yolov8.pt")
url = 'http://192.168.0.105/cam-mid.jpg'
cv2.namedWindow("Fire and Smoke Detection", cv2.WINDOW_NORMAL)

# API configuration
# api_url = "https://iot.nguyenduc.click/api/v1/fire/detection"
api_url = "http://192.168.0.104:3000/api/v1/fire/detection"
device_id = "ESP32_CAM_01"

# Biến để theo dõi lịch sử đám cháy
fire_history = []  # Lưu (timestamp, area, intensity)
last_api_call_time = 0
API_CALL_INTERVAL = 5  # Gửi API 5 giây/lần khi có cháy để tránh quá tải server

# Test API connection
try:
    test_response = requests.get(api_url.split('/api')[0])
    print(f"API connection test: {test_response.status_code}")
except Exception as conn_error:
    print(f"Cannot connect to API server: {conn_error}")

while True:
    try:
        # Đọc ảnh từ camera
        img_resp = urllib.request.urlopen(url)
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
        frame = cv2.imdecode(imgnp, -1)

        if frame is None:
            print("Failed to decode image")
            time.sleep(0.1)
            continue

        # Thực hiện detect
        results = model(frame)

        # Tạo frame có bounding box
        annotated_frame = frame.copy()
        fire_detected = False
        max_confidence = 0

        # Thêm biến để theo dõi tổng diện tích cháy
        total_fire_area = 0
        fire_boxes = []

        for result in results:
            for box in result.boxes:
                class_id = int(box.cls)
                confidence = float(box.conf)
                label = model.names[class_id]

                # Xác định loại đối tượng
                if label.lower() in ["fire"]:
                    display_label = "FIRE"
                    color = (0, 0, 255)  # Red
                    fire_detected = True

                    # Lấy thông tin bounding box
                    x1, y1, x2, y2 = map(int, box.xyxy[0])

                    # Tính diện tích
                    box_area = (x2 - x1) * (y2 - y1)
                    total_fire_area += box_area
                    fire_boxes.append((x1, y1, x2, y2, box_area))

                elif label.lower() in ["smoke"]:
                    display_label = "SMOKE"
                    color = (0, 255, 255)  # Yellow
                    fire_detected = True
                else:
                    display_label = "OTHER"
                    color = (0, 255, 0)  # Green

                # Cập nhật độ tin cậy cao nhất
                if confidence > max_confidence:
                    max_confidence = confidence

                # Vẽ bounding box và nhãn
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), color, 2)
                cv2.putText(annotated_frame,
                            f"{display_label} {confidence:.2f}",
                            (x1, y1 - 10),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.7, color, 2)

        # Phân tích mức độ cháy
        fire_percentage = 0
        fire_intensity = 0
        fire_growth_rate = 0
        fire_severity = "Không có cháy"

        if fire_detected:
            # Tính toán phần trăm diện tích cháy so với tổng diện tích khung hình
            frame_area = frame.shape[0] * frame.shape[1]
            fire_percentage = (total_fire_area / frame_area) * 100

            # Phân tích cường độ của lửa
            total_intensity = 0
            total_pixels = 0

            for x1, y1, x2, y2, _ in fire_boxes:
                # Trích xuất vùng lửa
                fire_region = frame[y1:y2, x1:x2]
                if fire_region.size == 0:
                    continue

                # Tính cường độ dựa trên kênh màu đỏ
                red_channel = fire_region[:, :, 2]  # BGR format, kênh 2 là R

                total_intensity += np.sum(red_channel)
                total_pixels += fire_region.size // 3  # 3 channels

            # Cường độ trung bình (0-255)
            if total_pixels > 0:
                avg_intensity = total_intensity / total_pixels
                # Chuẩn hóa về thang điểm 0-100
                fire_intensity = (avg_intensity / 255) * 100

            # Thêm vào lịch sử
            current_time = time.time()
            fire_history.append((current_time, fire_percentage, fire_intensity))

            # Giới hạn lịch sử chỉ lưu 1 phút gần nhất
            fire_history = [entry for entry in fire_history if current_time - entry[0] < 60]

            # Tính tốc độ phát triển của đám cháy
            if len(fire_history) > 1:
                first_entry = fire_history[0]
                time_diff = current_time - first_entry[0]
                area_diff = fire_percentage - first_entry[1]

                if time_diff > 0:
                    fire_growth_rate = area_diff / time_diff  # % mỗi giây

            # Đánh giá mức độ cháy dựa trên diện tích và cường độ
            if fire_percentage < 5 and fire_intensity < 60:
                fire_severity = "Chay nho"
            elif fire_percentage < 15 or (fire_percentage < 5 and fire_intensity >= 60):
                fire_severity = "Chay vua"
            else:
                fire_severity = "Chay lon"

            # Cập nhật mức độ cháy dựa trên tốc độ phát triển
            if fire_growth_rate > 1:  # Tăng hơn 1% mỗi giây
                fire_severity = "Cháy lớn và đang lan rộng nhanh!"

            # In ra thông tin phân tích để theo dõi
            print(f"------- PHÂN TÍCH ĐÁM CHÁY -------")
            print(f"Diện tích cháy: {fire_percentage:.2f}% của khung hình")
            print(f"Cường độ cháy: {fire_intensity:.2f}/100")
            print(f"Tốc độ phát triển: {fire_growth_rate:.4f}%/giây")
            print(f"Mức độ cháy: {fire_severity}")
            print(f"-----------------------------------")

        # Hiển thị thông tin lên frame
        if fire_detected:
            cv2.putText(annotated_frame, f"Muc Do: {fire_severity}",
                        (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.putText(annotated_frame, f"Dien tich: {fire_percentage:.2f}%",
                        (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
            cv2.putText(annotated_frame, f"Cuong do: {fire_intensity:.2f}",
                        (10, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

        # Hiển thị kết quả
        cv2.imshow('Fire and Smoke Detection', annotated_frame)

        # Gửi ảnh lên API nếu phát hiện lửa/khói và đủ thời gian giữa các lần gọi
        current_time = time.time()
        if fire_detected and (current_time - last_api_call_time >= API_CALL_INTERVAL):
            last_api_call_time = current_time

            # Lưu ảnh vào file tạm thời
            image_path = "detected_fire.jpg"
            cv2.imwrite(image_path, annotated_frame)

            # Tạo form data với các trường mới
            files = {'image': ('detected_fire.jpg', open(image_path, 'rb'), 'image/jpeg')}
            payload = {
                "device_id": device_id,
                "confidence_score": str(max_confidence),
                "is_fire": "true",
                "fire_severity": fire_severity,
                "fire_percentage": str(fire_percentage),
                "fire_intensity": str(fire_intensity),
                "fire_growth_rate": str(fire_growth_rate)
            }

            try:
                print(f"Sending request to: {api_url}")
                print(f"Payload: {payload}")
                print(f"File: {image_path}, Size: {os.path.getsize(image_path)} bytes")

                response = requests.post(api_url, data=payload, files=files)
                print(f"API Response: Status Code {response.status_code}")
                print(f"Response Content: {response.text}")

                # Đóng file sau khi gửi
                files['image'][1].close()
            except Exception as api_error:
                print(f"API Error: {api_error}")

        # Thoát khi nhấn 'q'
        if cv2.waitKey(5) == ord('q'):
            break

    except Exception as e:
        print(f"Error: {e}")
        time.sleep(1)

cv2.destroyAllWindows()