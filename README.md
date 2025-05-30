# Hệ thống Báo cháy Thông minh

Dự án của Nhóm 12 sử dụng ESP8266 để xây dựng hệ thống báo cháy thông minh, cho phép giám sát và cảnh báo cháy tự động. Hệ thống này thu thập dữ liệu từ các cảm biến và gửi cảnh báo đến máy chủ khi phát hiện sự cố, đồng thời hỗ trợ điều khiển buzzer từ xa để cảnh báo kịp thời.

## Tính năng

- **Kết nối WiFi**: ESP8266 tự động kết nối vào mạng WiFi để truyền dữ liệu liên tục.
- **Giám sát Cảm biến**:
  - Cảm biến Khí Gas: phát hiện rò rỉ khí gas.
  - Cảm biến Chất lượng Không khí: kiểm tra mức độ không khí xung quanh.
  - Cảm biến Ngọn lửa: phát hiện sự hiện diện của ngọn lửa.
- **Truyền Dữ liệu**: Dữ liệu cảm biến được gửi đến máy chủ định sẵn dưới dạng JSON.
- **Cảnh báo Buzzer từ xa**: Hỗ trợ điều khiển bật/tắt buzzer để cảnh báo từ xa.

## Yêu cầu phần cứng
- ESP8266
- Cảm biến Khí Gas, Cảm biến Chất lượng Không khí, và Cảm biến Ngọn lửa
- Buzzer

## Hướng dẫn Cài đặt
1. Kết nối các cảm biến và buzzer với ESP8266.
2. Cài đặt các thư viện cần thiết cho WiFi trong Arduino IDE.
3. Tải mã chương trình lên ESP8266 qua Arduino IDE.

## Cách thức Hoạt động
- ESP8266 kết nối với mạng WiFi và tự động giám sát các cảm biến.
- Khi phát hiện điều kiện nguy hiểm (khí gas, chất lượng không khí kém, hoặc ngọn lửa), ESP8266 gửi cảnh báo đến máy chủ và kích hoạt buzzer.
- Người dùng có thể điều khiển bật/tắt buzzer từ xa để đưa ra cảnh báo kịp thời.
