#include <Arduino.h>
#include <ESP32Servo.h>

// Định nghĩa các chân kết nối
#define TRIGGER_PIN 4    // Chân trigger của cảm biến siêu âm
#define ECHO_PIN 2       // Chân echo của cảm biến siêu âm
#define SERVO_PIN 11     // Chân điều khiển servo
#define IN1 7            // Chân điều khiển motor trái (A)
#define IN2 6            // Chân điều khiển motor trái (A)
#define IN3 9            // Chân điều khiển motor phải (B)
#define IN4 8            // Chân điều khiển motor phải (B)
#define EN_RIGHT 3       // Chân PWM cho motor phải
#define EN_LEFT 5        // Chân PWM cho motor trái

Servo SERVO_1;           // Đối tượng servo để điều khiển

// Biến toàn cục
float DISTANCE = 0;      // Khoảng cách đo được từ cảm biến siêu âm
float LEFT_DISTANCE = 0; // Khoảng cách bên trái
float RIGHT_DISTANCE = 0;// Khoảng cách bên phải
unsigned long DURATION_MICROSECOND = 0; // Thời gian phản hồi từ cảm biến siêu âm

// Hằng số
const int TIME_DELAY = 500;      // Thời gian chờ (ms) cho các hành động
const int SPEED_FORWARD = 150;   // Tốc độ tiến thẳng
const int SPEED_TURN = 255;      // Tốc độ khi quay
const float DISTANCE_LIMIT = 30.0; // Ngưỡng khoảng cách để phát hiện vật cản
const int SLOW_SPEED = 70;      // Tốc độ chậm khi gần vật cản

void setup() {
  // Cấu hình các chân cho motor
  pinMode(IN1, OUTPUT);    // Chân điều khiển motor trái (A)
  pinMode(IN2, OUTPUT);    // Chân điều khiển motor trái (A)
  pinMode(IN3, OUTPUT);    // Chân điều khiển motor phải (B)
  pinMode(IN4, OUTPUT);    // Chân điều khiển motor phải (B)
  pinMode(EN_LEFT, OUTPUT); // Chân PWM motor trái
  pinMode(EN_RIGHT, OUTPUT); // Chân PWM motor phải

  // Cấu hình cảm biến siêu âm
  pinMode(TRIGGER_PIN, OUTPUT); // Chân trigger của HC-SR04
  pinMode(ECHO_PIN, INPUT);     // Chân echo của HC-SR04

  // Khởi tạo servo
  SERVO_1.attach(SERVO_PIN);    // Kết nối servo với chân điều khiển
  SERVO_1.write(90);           // Đặt servo nhìn thẳng về phía trước

  Serial.begin(9600);           // Khởi tạo giao tiếp Serial để debug
}

// Đo khoảng cách bằng cảm biến siêu âm
float MEASURE_DISTANCE() {
  digitalWrite(TRIGGER_PIN, LOW);   // Đặt trigger thấp để reset
  delayMicroseconds(2);             // Chờ 2 micro giây
  digitalWrite(TRIGGER_PIN, HIGH);  // Gửi xung trigger
  delayMicroseconds(10);            // Duy trì xung trong 10 micro giây
  digitalWrite(TRIGGER_PIN, LOW);   // Tắt trigger

  DURATION_MICROSECOND = pulseIn(ECHO_PIN, HIGH, 30000); // Đọc thời gian phản hồi, timeout 30ms
  if (DURATION_MICROSECOND == 0) return 999; // Trả về 999 nếu cảm biến lỗi hoặc ngoài phạm vi
  DISTANCE = (DURATION_MICROSECOND * 0.0343 )/ 2.0; // Tính khoảng cách (cm)
  return DISTANCE;
}

// Điều khiển robot tiến thẳng với tốc độ tùy chỉnh
void GO_FORWARD(int speed = SPEED_FORWARD) {
  analogWrite(EN_LEFT, speed);   // Đặt tốc độ motor trái
  analogWrite(EN_RIGHT, speed);  // Đặt tốc độ motor phải
  digitalWrite(IN1, HIGH);       // Motor trái quay thuận
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);       // Motor phải quay thuận
  digitalWrite(IN4, LOW);
}

// Điều khiển robot lùi lại
void GO_BACKWARD() {
  analogWrite(EN_LEFT, SPEED_FORWARD); // Đặt tốc độ motor trái
  analogWrite(EN_RIGHT, SPEED_FORWARD); // Đặt tốc độ motor phải
  digitalWrite(IN1, LOW);        // Motor trái quay ngược
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);        // Motor phải quay ngược
  digitalWrite(IN4, HIGH);
}

// Điều khiển robot quay trái
void TURN_LEFT() {
  analogWrite(EN_LEFT, SPEED_TURN);  // Đặt tốc độ quay cho motor trái
  analogWrite(EN_RIGHT, SPEED_TURN); // Đặt tốc độ quay cho motor phải
  digitalWrite(IN1, LOW);            // Motor trái quay ngược
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);           // Motor phải quay thuận
  digitalWrite(IN4, LOW);
}

// Điều khiển robot quay phải
void TURN_RIGHT() {
  analogWrite(EN_LEFT, SPEED_TURN);  // Đặt tốc độ quay cho motor trái
  analogWrite(EN_RIGHT, SPEED_TURN); // Đặt tốc độ quay cho motor phải
  digitalWrite(IN1, HIGH);           // Motor trái quay thuận
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);            // Motor phải quay ngược
  digitalWrite(IN4, HIGH);
}

// Kiểm tra khoảng cách bên trái
float CHECK_LEFT() {
  SERVO_1.write(180);     // Xoay servo sang trái (150 độ)
  delay(TIME_DELAY);       // Chờ servo di chuyển
  float TEMP = MEASURE_DISTANCE(); // Đo khoảng cách bên trái
  SERVO_1.write(90);      // Đưa servo về vị trí ban đầu
  delay(300);              // Chờ servo quay lại
  return TEMP;
}

// Kiểm tra khoảng cách bên phải
float CHECK_RIGHT() {
  SERVO_1.write(0);      // Xoay servo sang phải (30 độ)
  delay(TIME_DELAY);       // Chờ servo di chuyển
  float TEMP = MEASURE_DISTANCE(); // Đo khoảng cách bên phải
  SERVO_1.write(90);      // Đưa servo về vị trí ban đầu
  delay(300);              // Chờ servo quay lại
  return TEMP;
}

// Dừng toàn bộ motor
void STOP() {
  analogWrite(EN_RIGHT, 0);  // Tắt PWM motor phải
  analogWrite(EN_LEFT, 0);   // Tắt PWM motor trái
  digitalWrite(IN1, LOW);    // Ngắt motor trái
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);    // Ngắt motor phải
  digitalWrite(IN4, LOW);
}

// Vòng lặp chính điều khiển robot
void loop() {
  DISTANCE = MEASURE_DISTANCE(); // Đo khoảng cách phía trước
  if (DISTANCE >= DISTANCE_LIMIT) { 
    GO_FORWARD(); // Tiến thẳng nếu không có vật cản
  } else if (DISTANCE >= DISTANCE_LIMIT && DISTANCE <40) { 
    GO_FORWARD(SLOW_SPEED); // Giảm tốc độ khi gần vật cản
  }
   else { 
    STOP();
    delay(500); // Dừng lại khi gặp vật cản
    LEFT_DISTANCE = CHECK_LEFT(); // Kiểm tra khoảng cách bên trái
    RIGHT_DISTANCE = CHECK_RIGHT(); // Kiểm tra khoảng cách bên phải
    
   if (DISTANCE<30 || RIGHT_DISTANCE<DISTANCE_LIMIT && LEFT_DISTANCE<DISTANCE_LIMIT) {
    GO_BACKWARD();
    delay(1500);
    STOP();
    delay(500);
  }
     else if (RIGHT_DISTANCE >= LEFT_DISTANCE && LEFT_DISTANCE>=DISTANCE_LIMIT) {
      TURN_LEFT(); // Quay phải nếu bên phải thoáng hơn
      delay(700);
      STOP();
      delay(500);
    } else {
      TURN_RIGHT(); // Quay trái nếu bên trái thoáng hơn
      delay(700);
      STOP();
      delay(500);
    }
  }}