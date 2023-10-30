#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Bounce2.h>
#include <AccelStepper.h>

// PCA9685 address and I2C instance
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

//Servo---------------------------------------------------------------------------
int eje1 = 90; // 0-180
int eje2 = 90; // 180-0
int eje3 = 0;
int eje4 = 0;

//Motor---------------------------------------------------------------------------
//definizione delle costanti dei pin di Arduino
//กำหนดค่าคงที่ ledEnable ให้เป็น 13 ซึ่งเป็นขาของ LED บนบอร์ด Arduino ที่จะใช้แสดงสถานะของการเปิดหรือปิดมอเตอร์.
const int ledEnable = 13; 
//กำหนดค่าคงที่ pinSwEnable เป็น 7 ซึ่งเป็นขาที่เชื่อมต่อกับสวิตช์ในโมดูลจอยสติกต์เพื่อเปิดหรือปิดการควบคุม.
const int pinSwEnable = 7;  
//กำหนดค่าคงที่ pinEnable เป็น 8 ซึ่งเป็นขาที่ใช้ควบคุมสถานะ "ENABLE" ของไดรเวอร์ A4988 โดยการต่อขา ENABLE ของทั้งสองมอเตอร์ A4988 ในโหมดนี้.
const int pinEnable = 8; 
//กำหนดค่าคงที่ debounceDelay ให้เป็น 10 มิลลิวินาทีสำหรับการดีบอนซ์ของสวิตช์.
unsigned long debounceDelay = 10; 

const int jX = A0;  
const int stepX = 3;  
const int dirX = 4; 
//ประกาศตัวแปรสำหรับควบคุมความเร็วแกน X และอ่านค่าตำแหน่งแกน X จากจอยสติกต์.
long speedX, valX, mapX;  

const int jY = A1;  
const int stepY = 5;  
const int dirY = 6; 
//ประกาศตัวแปรสำหรับควบคุมความเร็วแกน Y และอ่านค่าตำแหน่งแกน Y จากจอยสติกต์.
long speedY, valY, mapY;  

//กำหนดค่าคงที่ maxSpeed เป็น 1000 สำหรับความเร็วสูงสุดของมอเตอร์.
const int maxSpeed = 1000;  
//กำหนดค่าคงที่ minSpeed เป็น 0 สำหรับความเร็วต่ำสุดของมอเตอร์.
const int minSpeed = 0; 
//กำหนดค่าคงที่ accelerazione เป็น 50.0 สำหรับความเร็วในการเร่ง.
const float accelerazione = 50.0; 

//กำหนดค่าคงที่ treshold เป็น 30 สำหรับการกำหนดจุด "Stai fermo" ในการอ่านค่าจากจอยสติกต์.
const int treshold = 30;  
//ประกาศตัวแปรสำหรับการคำนวณของขีดจำกัด "Stai fermo".
long tresholdUp, tresholdDown;  

//ประกาศตัวแปรสำหรับการควบคุมการเคลื่อนไหวและสถานะการเปิดหรือปิด.
boolean abilitato, muoviX, muoviY, enable;  //variabili di gestione dei movimenti
//สร้างอ็อบเจ็กต์ Bounce สำหรับการจัดการกับปุ่ม "เปิด/ปิด".
Bounce btnEnable = Bounce();  //istanzia un bottone dalla libreria Bounce

// สร้างอ็อบเจ็กต์ AccelStepper สำหรับควบคุมมอเตอร์แกน X และ Y โดยใช้ไดรเวอร์ของแอ็คเซลเรร์เปอร์.
AccelStepper motoreX(AccelStepper::DRIVER, stepX, dirX);
AccelStepper motoreY(AccelStepper::DRIVER, stepY, dirY);

void setup() {

//Servo---------------------------------------------------------------------------
  Serial.begin(9600);
  Serial.println("Hello Setup");
  
  pwm.begin();
  pwm.setPWMFreq(60);  // ตั้งค่าความถี่ PWM (60 Hz เป็นค่าพื้นฐาน)

  // ตั้งค่าช่องขาของ PCA9685 สำหรับแต่ละ servo
  int servo1_channel = 0;  // ให้ช่อง 0 สำหรับ servo1
  int servo2_channel = 4;  // ให้ช่อง 1 สำหรับ servo2
  int servo3_channel = 8;  // ให้ช่อง 2 สำหรับ servo3
  int servo4_channel = 15;  // ให้ช่อง 3 สำหรับ servo4

  // ตั้งค่าตำแหน่งเริ่มต้นของแต่ละ servo
  int servo1_position = map(eje1, 0, 180, 150, 600); // แปลงค่า eje1 เป็นช่วง PWM
  int servo2_position = map(eje2, 0, 180, 150, 600); // แปลงค่า eje2 เป็นช่วง PWM
  int servo3_position = map(eje3, 0, 180, 150, 600); // แปลงค่า eje3 เป็นช่วง PWM
  int servo4_position = map(eje4, 0, 180, 150, 600); // แปลงค่า eje4 เป็นช่วง PWM

  // กำหนดตำแหน่งเริ่มต้นของแต่ละ servo
  pwm.setPWM(servo1_channel, 0, servo1_position);
  pwm.setPWM(servo2_channel, 0, servo2_position);
  pwm.setPWM(servo3_channel, 0, servo3_position);
  pwm.setPWM(servo4_channel, 0, servo4_position);

//Motor--------------------------------------------------------------------------
//inizializza valori
  //ถูกกำหนดเป็น 0 ซึ่งเป็นความเร็วเริ่มต้นของโมเตอร์ในแกน X และ Y ตามลำดับ.
  speedX = speedY = 0;
  //เพื่อปิดการเปิดใช้งานการควบคุมโมเตอร์.
  enable = false;

  pinMode(ledEnable, OUTPUT);
  pinMode(pinEnable, OUTPUT);

  pinMode(pinSwEnable, INPUT_PULLUP); 

  digitalWrite(ledEnable, enable);
  // ไดรเวอร์ A4988 ปิดใช้งานคำสั่งมอเตอร์หากได้รับสัญญาณสูงบนพิน ENABLE ด้วยเหตุนี้ค่าจึงอยู่ตรงข้ามกับค่าของ LED
  digitalWrite(pinEnable, !enable); 

  //กำหนดการใช้งานสวิตช์โดยใช้ไลบรารี Bounce โดยการเชื่อม btnEnable กับขา pinSwEnable และกำหนดค่า interval สำหรับการดี-บาวน์ (debounce) ด้วย 
  btnEnable.attach(pinSwEnable);
  btnEnable.interval(debounceDelay);

  //คำนวณช่วงค่าที่จะถูกพิจารณาว่าตำแหน่งของจอยสติกอยู่ในสถานะ "คงที่" โดยใช้ tresholdDown และ tresholdUp. สถานะ "คงที่" หมายถึงตำแหน่งที่จอยสติกอยู่ในส่วนกลางของช่วงค่าความเร็วของการเคลื่อนที่.
  tresholdDown = (maxSpeed / 2) - treshold;
  tresholdUp = (maxSpeed / 2) + treshold;

  // กำหนดค่าพารามิเตอร์ของเครื่องยนต์
  motoreX.setMaxSpeed(maxSpeed);
  motoreX.setSpeed(minSpeed);
  motoreX.setAcceleration(accelerazione);

  motoreY.setMaxSpeed(maxSpeed);
  motoreY.setSpeed(minSpeed);
  motoreY.setAcceleration(accelerazione);

}

void loop() {

//Servo--------------------------------------------------------------------------
  // อ่านค่า X และ Y จาก Joystick Module
  int xValue = analogRead(A2);  // ตัวอย่าง: A0 เป็นขาที่ต่อกับ X-axis
  int yValue = analogRead(A3);  // ตัวอย่าง: A1 เป็นขาที่ต่อกับ Y-axis

  int speed = 1; // ความเร็วในการหมุน (เปลี่ยนค่าตามความต้องการ)

   // SERVO 1 (เเกน X)
  if (xValue < 200 && eje1 < 150) {
    eje1 += speed; // เพิ่มค่า eje1 ขึ้น 1
    Serial.println("eje1 : ");
    Serial.println(eje1);
    int servo1_position = map(eje1, 0, 180, 150, 600); // แปลงค่า eje1 เป็นช่วง PWM
    pwm.setPWM(0, 0, servo1_position); // ตั้งค่า PWM ใหม่
  }
  if (xValue > 600 && eje1 > 0) {
    eje1 -= speed; // ลดค่า eje1 ลง 1
    Serial.println("eje1 : ");
    Serial.println(eje1);
    int servo1_position = map(eje1, 0, 180, 150, 600); // แปลงค่า eje1 เป็นช่วง PWM
    pwm.setPWM(0, 0, servo1_position); // ตั้งค่า PWM ใหม่
  }

  // SERVO 2 (เเกน X)
  if (xValue < 200 && eje2 > 0) {
    eje2 -= speed; // เพิ่มค่า eje2 ขึ้น 1
    Serial.println("eje2 : ");
    Serial.println(eje2);
    int servo2_position = map(eje2, 0, 180, 150, 600); // แปลงค่า eje2 เป็นช่วง PWM
    pwm.setPWM(4, 0, servo2_position); // ตั้งค่า PWM ใหม่
  }
  if (xValue > 600 && eje2 < 150) {
    eje2 += speed; // ลดค่า eje2 ลง 1
    Serial.println("eje2 : ");
    Serial.println(eje2);
    int servo2_position = map(eje2, 0, 180, 150, 600); // แปลงค่า eje2 เป็นช่วง PWM
    pwm.setPWM(4, 0, servo2_position); // ตั้งค่า PWM ใหม่
  }

  // SERVO 3 (เเกน Y)
  if (yValue < 200 && eje3 < 209) {
    eje3 += 1; // เพิ่มค่า eje3 ขึ้น 1
    int servo3_position = map(eje3, 0, 150, 180, 600); // แปลงค่า eje3 เป็นช่วง PWM
    pwm.setPWM(8, 0, servo3_position); // ตั้งค่า PWM ใหม่
  }
  if (yValue > 600 && eje3 > 0) {
    eje3 -= 1; // ลดค่า eje3 ลง 1
    int servo3_position = map(eje3, 0, 150, 180, 600); // แปลงค่า eje3 เป็นช่วง PWM
    pwm.setPWM(8, 0, servo3_position); // ตั้งค่า PWM ใหม่
  }

  // SERVO 4 (เเกน Y)
  if (yValue < 200 && eje4 < 209) {
    eje4 += 1; // เพิ่มค่า eje4 ขึ้น 1
    int servo4_position = map(eje4, 0, 150, 180, 600); // แปลงค่า eje4 เป็นช่วง PWM
    pwm.setPWM(15, 0, servo4_position); // ตั้งค่า PWM ใหม่
  }
  if (yValue > 600 && eje4 > 0) {
    eje4 -= 1; // ลดค่า eje4 ลง 1
    int servo4_position = map(eje4, 0, 150, 180, 600); // แปลงค่า eje4 เป็นช่วง PWM
    pwm.setPWM(15, 0, servo4_position); // ตั้งค่า PWM ใหม่
  }

    delay(15);

//Motor----------------------------------------------------------------------------
//ดำเนินการควบคุมและอ่านฟังก์ชันของปุ่มที่กำหนดสถานะการเปิดใช้งาน
  checkEnable();

  digitalWrite(ledEnable, enable);  //แสดงสถานะการเปิดใช้งานผ่าน LED ที่ขา 13
  digitalWrite(pinEnable, !enable); //ตั้งค่าตรงกันข้ามบนพิน ENABLE ของไดรเวอร์

  //ทำการอ่านค่าแบบอะนาล็อกที่มาจากโพเทนชิโอมิเตอร์ของจอยสติ๊ก
  valX = analogRead(jX);
  valY = analogRead(jY);

  //แมปค่าการอ่านเป็นฟังก์ชันของความเร็วต่ำสุดและสูงสุด
  mapX = map(valX, 0, 1023, minSpeed, maxSpeed);
  mapY = map(valY, 0, 1023, minSpeed, maxSpeed);

  // เรียกใช้ฟังก์ชันควบคุมมอเตอร์
  pilotaMotori(mapX, mapY);

}

//ฟังก์ชันที่ใช้ในการควบคุมการเคลื่อนไหวของมอเตอร์แกน X และ Y.
void pilotaMotori(long mapX, long mapY) {

  if (mapX <= tresholdDown) {
    //x ถอยหลัง
    speedX = -map(mapX, tresholdDown, minSpeed,   minSpeed, maxSpeed);
    muoviX = true;
  } else if (mapX >= tresholdUp) {
    //x ก้าวไปข้างหน้า
    speedX = map(mapX,  maxSpeed, tresholdUp,  maxSpeed, minSpeed);
    muoviX = true;
  } else {
    //x ยังคงอยู่นิ่งๆ
    speedX = 0;
    muoviX = false;
  }

  if (mapY <= tresholdDown) {
    //y ลงไป
    speedY = -map(mapY, tresholdDown, minSpeed,   minSpeed, maxSpeed);
    muoviY = true;
  } else if (mapY >= tresholdUp) {
    //y ขึ้นไป
    speedY = map(mapY,  maxSpeed, tresholdUp,  maxSpeed, minSpeed);
    muoviY = true;
  } else {
    //y ยังคงอยู่
    speedY = 0;
    muoviY = false;
  }

  if (muoviX) {
    motoreX.setSpeed(speedX);
    motoreX.run();
  } else {
    motoreX.stop();
  }

  if (muoviY) {
    motoreY.setSpeed(speedY);
    motoreY.run();
  } else {
    motoreY.stop();
  }
}

void checkEnable() {

  btnEnable.update();

  if (btnEnable.fell()) {
    enable = !enable;
  }

}

