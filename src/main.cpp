#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

#define IN1 14
#define IN2 15
#define IN3 16
#define IN4 17
#define ENA 18
#define ENB 21
#define TRIG 10
#define ECHO 9
#define SERVO_PIN 4

Servo servo1;
float t;
float w, a, d;
String now = "X";
String last = "";

const char* ssid = "VanTuan";
const char* password = "11111111";
const char* mqtt_server = "broker.emqx.io"; 

WiFiClient espClient;
PubSubClient client(espClient);


void W() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void S() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void D() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void A() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void STOP() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}


float kc() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  t = pulseIn(ECHO, HIGH, 30000); 
  float distance = (0.0343 * t) / 2;
  return distance;
}

float kc_a() {
  servo1.write(180);
  delay(200);
  float TEMP = kc();
  servo1.write(90);
  delay(200);
  return TEMP;
}

float kc_d() {
  servo1.write(0);
  delay(200);
  float TEMP = kc();
  servo1.write(90);
  delay(200);
  return TEMP;
}


void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];

  Serial.print("Nhận lệnh từ MQTT: ");
  Serial.println(message);

  if (String(topic) == "xe1/control") {
    if (message == "W") { W(); now = "W"; }
    else if (message == "S") { S(); now = "S"; }
    else if (message == "A") { A(); now = "A"; }
    else if (message == "D") { D(); now = "D"; }
    else if (message == "X") { STOP(); now = "X"; }
  }
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Kết nối MQTT...");
    if (client.connect("ESP32S3")) {
      Serial.println("OK");
      client.subscribe("xe1/control");
    } else {
      Serial.print("Thất bại, mã lỗi: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  servo1.attach(SERVO_PIN);
  servo1.write(90);

  Serial.print("Kết nối WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nĐã kết nối WiFi!");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  w = kc();
  if (w >= 30) {
    W();
    now = "W";
  } else {
    STOP();
    now = "X";
  }
  delay(300);

  a = kc_a();
  d = kc_d();

  if (w < 25 && a < 20 && d < 20) {
    S(); delay(1000); now = "S";
  } else if (a > d && d > 20) {
    A(); delay(700); now = "A";
  } else if (d > a && a > 20) {
    D(); delay(700); now = "D";
  }

  
  if (now != last) {
    last = now;
    if (now == "W") client.publish("esp32test1", "XE ĐANG ĐI THẲNG");
    else if (now == "A") client.publish("esp32test1", "XE QUẶT TRÁI");
    else if (now == "S") client.publish("esp32test1", "XE ĐI LÙI");
    else if (now == "D") client.publish("esp32test1", "XE QUẶT PHẢI");
    else if (now == "X") client.publish("esp32test1", "XE DỪNG");
  }
}
