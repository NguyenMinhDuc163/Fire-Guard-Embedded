#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>  // Thư viện để tạo và xử lý JSON

// Danh sách các cấu hình WiFi
const char* ssidList[] = {"skibidi 2.4", "NguyenDuc", "PLZ"};
const char* passwordList[] = {"Tu9denmot", "tumotden8", "11110000"};

// Cấu hình địa chỉ IP tĩnh
IPAddress local_IP(192, 168, 1, 50);     // Địa chỉ IP tĩnh mong muốn
IPAddress gateway(192, 168, 1, 1);       // Địa chỉ gateway của router
IPAddress subnet(255, 255, 255, 0);      // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);        // DNS chính (Google DNS)
IPAddress secondaryDNS(8, 8, 4, 4);      // DNS phụ (Google DNS)

// URL của server để gửi dữ liệu cảm biến
const char* serverUrl = "http://20.255.153.8:3000/api/v1/sensors/data";

// Định nghĩa chân kết nối của các cảm biến và còi báo
#define GAS_SENSOR_PIN 2
#define AIR_SENSOR_PIN 14
#define FLAME_SENSOR_PIN 12
#define BUZZER_PIN 13

// Khai báo biến cho kết nối WiFi và server HTTP
WiFiClient client;
ESP8266WebServer server(80);  // Tạo HTTP server trên ESP8266, lắng nghe tại cổng 80
unsigned long lastSendTime = 0;  // Biến lưu thời gian gửi dữ liệu lần cuối

// Biến để điều khiển còi từ API
bool buzzerDisabled = false;
unsigned long lastDisabledTime = 0;
const unsigned long disableDuration = 30000;  // 30 giây

// Hàm thiết lập các chân của cảm biến và còi báo
void pin_mode() {
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(AIR_SENSOR_PIN, INPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

// Hàm xử lý yêu cầu bật/tắt còi báo từ server
void handleToggleBuzzer() {
  if (server.hasArg("status")) {  // Kiểm tra tham số "status"
    if (server.arg("status") == "off") {
      buzzerDisabled = true;
      lastDisabledTime = millis();
      digitalWrite(BUZZER_PIN, LOW);  // Tắt còi báo
      server.send(200, "application/json", "{\"message\":\"Buzzer turned off by API\"}");
    } else if (server.arg("status") == "on") {
      buzzerDisabled = false;
      server.send(200, "application/json", "{\"message\":\"Buzzer can be turned on by sensor condition\"}");
    }
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing 'status' parameter\"}");
  }
}

// Hàm kết nối tới một trong các mạng WiFi trong danh sách
void connectToWiFi() {
  bool isConnected = false;  // Biến kiểm tra trạng thái kết nối

  for (int i = 0; i < sizeof(ssidList) / sizeof(ssidList[0]); i++) {
    WiFi.begin(ssidList[i], passwordList[i]);
    Serial.print("Đang kết nối tới WiFi: ");
    Serial.println(ssidList[i]);

    // Chờ kết nối trong tối đa 10 giây
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 10) {  // Thử kết nối trong tối đa 10 giây
      delay(1000);
      Serial.print(".");
      retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      isConnected = true;
      Serial.println("\nKết nối thành công!");
      Serial.print("Địa chỉ IP: ");
      Serial.println(WiFi.localIP());
      break;
    } else {
      Serial.println("\nKhông thể kết nối.");
    }
  }

  if (!isConnected) {
    Serial.println("Không có mạng WiFi nào khả dụng. Vui lòng kiểm tra cấu hình.");
  }
}

// Hàm khởi tạo ESP8266
void setup() {
  Serial.begin(115200);  // Khởi tạo Serial với tốc độ 115200

  // Cấu hình IP tĩnh cho ESP8266
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure Static IP");
  }

  // Kết nối WiFi từ danh sách cấu hình
  connectToWiFi();
  
  pin_mode();  // Thiết lập chế độ cho các chân
  digitalWrite(BUZZER_PIN, LOW);  // Tắt còi báo ban đầu

  // Thiết lập endpoint /toggle_buzzer để bật/tắt còi báo
  server.on("/toggle_buzzer", HTTP_POST, handleToggleBuzzer);
  server.begin();  // Bắt đầu server

  // Còi kêu báo hiệu thiết lập thành công
  digitalWrite(BUZZER_PIN, HIGH);  // Bật còi báo
  delay(500);                      // Còi kêu trong 0.5 giây
  digitalWrite(BUZZER_PIN, LOW);   // Tắt còi báo
}

// Hàm gửi dữ liệu cảm biến đến server
void sendData(int gasValue, int airValue, int flaValue, bool buzzerStatus) {
  if (WiFi.status() == WL_CONNECTED) {  // Kiểm tra kết nối WiFi
    HTTPClient http;
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Tạo JSON để gửi dữ liệu cảm biến
    StaticJsonDocument<200> doc;
    doc["device_id"] = "esp8266_001";
    doc["flame_sensor"] = flaValue;
    doc["mq2_gas_level"] = gasValue;
    doc["mq135_air_quality"] = airValue;
    doc["buzzer_status"] = buzzerStatus;
    doc["timestamp"] = "2024-10-13T14:00:00Z";

    // Chuyển đổi JSON thành chuỗi để gửi
    String jsonData;
    serializeJson(doc, jsonData);

    // Gửi dữ liệu POST đến server
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error on sending POST: " + String(httpResponseCode));
    }

    http.end();  // Kết thúc kết nối HTTP
  } else {
    Serial.println("WiFi not connected");
  }
}

void loop() {
  // Đọc dữ liệu từ các cảm biến
  int gasValue = analogRead(GAS_SENSOR_PIN);
  int airValue = analogRead(AIR_SENSOR_PIN);
  int flaValue = analogRead(FLAME_SENSOR_PIN);

  // Kiểm tra thời gian tắt còi tạm thời
  if (buzzerDisabled && millis() - lastDisabledTime >= disableDuration) {
    buzzerDisabled = false;  // Hết thời gian tắt tạm thời
  }

  // Cập nhật trạng thái còi báo dựa vào điều kiện và trạng thái tắt tạm thời
  bool buzzerStatus = ((flaValue < 500)|| (airValue != 0 || gasValue != 0)) && !buzzerDisabled;
  digitalWrite(BUZZER_PIN, buzzerStatus ? HIGH : LOW);

  // In giá trị cảm biến lên Serial Monitor
  Serial.printf("Gas value = %d\nAir value = %d\nFlame value = %d\n", gasValue, airValue, flaValue);
  Serial.println("-------------------");

  // Kiểm tra thời gian để gửi dữ liệu định kỳ hoặc khi có cảnh báo
  unsigned long currentTime = millis();
  if (airValue != 0 || gasValue != 0 ) {
    // Gửi dữ liệu ngay lập tức nếu có giá trị không bình thường từ cảm biến khí
    sendData(gasValue, airValue, flaValue, buzzerStatus);
    lastSendTime = currentTime;  // Cập nhật thời gian gửi cuối
  } else if (buzzerStatus || currentTime - lastSendTime >= 30000) {
    // Gửi dữ liệu định kỳ mỗi 30 giây hoặc khi có tín hiệu cháy
    sendData(gasValue, airValue, flaValue, buzzerStatus);
    lastSendTime = currentTime;  // Cập nhật thời gian gửi cuối
  }

  // Xử lý các yêu cầu HTTP từ server Express.js
  server.handleClient();

  delay(500);  // Thời gian chờ trước khi lặp lại
}