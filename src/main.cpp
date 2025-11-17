#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#define IN1 14
#define IN2 15
#define IN3 16
#define IN4 17
#define ENA 21
#define ENB 18
#define TRIG 13
#define ECHO 12
#define SERVO_PIN 4

Servo servo1;
float t, w, a, d;
String now = "X";
String last = "";
bool autoMode = true; 


const char* ssid = "TrGiang";
const char* password = "2392005@";
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

void W() {  // Tiến
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void S() {  // Lùi
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}
void D() {  // Rẽ phải
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void A() {  // Rẽ trái
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}
void STOP() {  // Dừng
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// ---------------- HÀM ĐO KHOẢNG CÁCH ----------------
float kc() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  t = pulseIn(ECHO, HIGH, 30000); // timeout 30ms
  return ((0.0343 * t) / 2); // khoảng cách cm
}

// đo bên trái
float kc_a() {
  servo1.write(180); delay(500);
  float TEMPA = kc();
  servo1.write(90); delay(300);
  return TEMPA;
}

// đo bên phải
float kc_d() {
  servo1.write(0); delay(500);
  float TEMPD = kc();
  servo1.write(90); delay(300);
  return TEMPD;
}

// ---------------- HÀM XỬ LÝ MQTT ----------------
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];

  Serial.print("Nhận lệnh từ MQTT: ");
  Serial.println(message);

  if (String(topic) == "xe1/control") {
    if (message == "AUTO_ON") {
      autoMode = true;
      client.publish("esp32test1", "ĐÃ CHUYỂN SANG CHẾ ĐỘ TỰ ĐỘNG");
    } 
    else if (message == "AUTO_OFF") {
      autoMode = false;
      STOP();
      client.publish("esp32test1", "ĐÃ CHUYỂN SANG CHẾ ĐỘ THỦ CÔNG");
    } 
    else if (!autoMode) {  // chỉ điều khiển tay khi chưa bật AUTO
      if (message == "W") { W(); now = "W"; }
      else if (message == "S") { S(); now = "S"; }
      else if (message == "A") { A(); now = "A"; }
      else if (message == "D") { D(); now = "D"; }
      else if (message == "X") { STOP(); now = "X"; }
    }
  }
}

// ---------------- HÀM KẾT NỐI MQTT ----------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Kết nối MQTT...");
    if (client.connect("ESP32S3_Client")) {
      Serial.println("OK");
      client.subscribe("xe1/control");
      client.publish("esp32test1", "ESP32S3 ĐÃ KẾT NỐI THÀNH CÔNG");
    } else {
      Serial.print("Thất bại, mã lỗi: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  
  // Khởi tạo chân
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  
  servo1.attach(SERVO_PIN);
  servo1.write(90);

  // Kết nối Wi-Fi
  Serial.print("Kết nối WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ Đã kết nối WiFi!");
  Serial.println(WiFi.localIP());

  // Kết nối MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ---------------- LOOP ----------------
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (autoMode) {
    w = kc(); // đo phía trước

    if (w >= 30) {
      W(); now = "W";  // đi thẳng
    } else {
      STOP(); now = "X"; delay(200);

      a = kc_a(); // đo trái
      d = kc_d(); // đo phải

      if (a > d && a > 20) {
        A(); now = "A";
      } else if (d >= a && d > 20) {
        D(); now = "D";
      } else {
        S(); now = "S";
      }
      delay(500);
    }

    // Chỉ gửi lên MQTT khi có thay đổi hành động
    if (now != last) {
      last = now;
      String msg;
      if (now == "W") msg = "🚗 XE ĐI THẲNG (AUTO)";
      else if (now == "A") msg = "↩️ XE RẼ TRÁI (AUTO)";
      else if (now == "S") msg = "⬅️ XE LÙI (AUTO)";
      else if (now == "D") msg = "↪️ XE RẼ PHẢI (AUTO)";
      else if (now == "X") msg = "⛔ XE DỪNG (AUTO)";
      client.publish("esp32test1", msg.c_str());
    }
  }

  Serial.print("Khoảng cách trước: ");
  Serial.println(w);
  delay(200);
}