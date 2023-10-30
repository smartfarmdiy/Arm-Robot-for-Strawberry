
/*
 * Controllo di due motori passo passo con Arduino e un Joystick
 *
 * Autore  : Andrea Lombardo
 * Web     : http://www.lombardoandrea.com
 * Post    : http://wp.me/p27dYH-KQ
 */ 

//Inclusione delle librerie
#include <AccelStepper.h>
#include <Bounce2.h>

//definizione delle costanti dei pin di Arduino
//กำหนดค่าคงที่ ledEnable ให้เป็น 13 ซึ่งเป็นขาของ LED บนบอร์ด Arduino ที่จะใช้แสดงสถานะของการเปิดหรือปิดมอเตอร์.
const int ledEnable = 13; //il led on board ci mostrerà lo stato di attivazione dei motori
//กำหนดค่าคงที่ pinSwEnable เป็น 7 ซึ่งเป็นขาที่เชื่อมต่อกับสวิตช์ในโมดูลจอยสติกต์เพื่อเปิดหรือปิดการควบคุม.
const int pinSwEnable = 7;  //il bottone presente nel modulo joystick che abilita o disabilita il controllo
//กำหนดค่าคงที่ pinEnable เป็น 8 ซึ่งเป็นขาที่ใช้ควบคุมสถานะ "ENABLE" ของไดรเวอร์ A4988 โดยการต่อขา ENABLE ของทั้งสองมอเตอร์ A4988 ในโหมดนี้.
const int pinEnable = 8;  //i pin che comandano lo stato ENABLE dei driver A4988 sono in collegati in serie per questo basta un solo pin per gestirli entrambi
//กำหนดค่าคงที่ debounceDelay ให้เป็น 10 มิลลิวินาทีสำหรับการดีบอนซ์ของสวิตช์.
unsigned long debounceDelay = 10; //millisecondi per il debonuce del bottone

const int jX = A0;  //pin analogico che legge i valori per le X
const int stepX = 3;  //pin digitale che invia i segnali di STEP al driver delle X
const int dirX = 4; //pin digitale che invia il segnale DIREZIONE al driver delle X
//ประกาศตัวแปรสำหรับควบคุมความเร็วแกน X และอ่านค่าตำแหน่งแกน X จากจอยสติกต์.
long speedX, valX, mapX;  //variabili di gestione movimenti motore X

const int jY = A1;  //pin analogico che legge i valori per le Y
const int stepY = 5;  //pin digitale che invia i segnali di STEP al driver delle Y
const int dirY = 6; //pin digitale che invia il segnale DIREZIONE al driver delle Y
//ประกาศตัวแปรสำหรับควบคุมความเร็วแกน Y และอ่านค่าตำแหน่งแกน Y จากจอยสติกต์.
long speedY, valY, mapY;  //variabili di gestione movimenti motore Y

//variabili utilizzate dalla libreria AccelStepper
//กำหนดค่าคงที่ maxSpeed เป็น 1000 สำหรับความเร็วสูงสุดของมอเตอร์.
const int maxSpeed = 1000;  //stando alla documentazione della libreria questo valore può essere impostato fino a 4000 per un Arduino UNO
//กำหนดค่าคงที่ minSpeed เป็น 0 สำหรับความเร็วต่ำสุดของมอเตอร์.
const int minSpeed = 0; //velocità minima del motore
//กำหนดค่าคงที่ accelerazione เป็น 50.0 สำหรับความเร็วในการเร่ง.
const float accelerazione = 50.0; //numero di step al secondo in accelerazione

//กำหนดค่าคงที่ treshold เป็น 30 สำหรับการกำหนดจุด "Stai fermo" ในการอ่านค่าจากจอยสติกต์.
const int treshold = 30;  //la lettura dei potenziometri non è mai affidabile al 100%, questo valore aiuta a determinare il punto da considerare come "Stai fermo" nei movimenti
//ประกาศตัวแปรสำหรับการคำนวณของขีดจำกัด "Stai fermo".
long tresholdUp, tresholdDown;  //variabili di servizio per espletare il compito descritto sopra

//ประกาศตัวแปรสำหรับการควบคุมการเคลื่อนไหวและสถานะการเปิดหรือปิด.
boolean abilitato, muoviX, muoviY, enable;  //variabili di gestione dei movimenti

//สร้างอ็อบเจ็กต์ Bounce สำหรับการจัดการกับปุ่ม "เปิด/ปิด".
Bounce btnEnable = Bounce();  //istanzia un bottone dalla libreria Bounce

//istanzia i motori
// สร้างอ็อบเจ็กต์ AccelStepper สำหรับควบคุมมอเตอร์แกน X และ Y โดยใช้ไดรเวอร์ของแอ็คเซลเรร์เปอร์.
AccelStepper motoreX(AccelStepper::DRIVER, stepX, dirX);
AccelStepper motoreY(AccelStepper::DRIVER, stepY, dirY);

void setup() {
  //inizializza valori
  //ถูกกำหนดเป็น 0 ซึ่งเป็นความเร็วเริ่มต้นของโมเตอร์ในแกน X และ Y ตามลำดับ.
  speedX = speedY = 0;
  //เพื่อปิดการเปิดใช้งานการควบคุมโมเตอร์.
  enable = false;

  //definizione delle modalità dei pin
  pinMode(ledEnable, OUTPUT);
  pinMode(pinEnable, OUTPUT);

  pinMode(pinSwEnable, INPUT_PULLUP); //l'input dello switch ha bisogno di essere settato come INPUT_PULLUP

  digitalWrite(ledEnable, enable);
  digitalWrite(pinEnable, !enable); //I driver A4988 disabilitano i comandi al motore se sul pin ENABLE ricevono un segnale HIGH per questo motivo il valore è opposto a quello del LED

  //configura il bottone del joystick utilizzando la libreria Bounce
  //กำหนดการใช้งานสวิตช์โดยใช้ไลบรารี Bounce โดยการเชื่อม btnEnable กับขา pinSwEnable และกำหนดค่า interval สำหรับการดี-บาวน์ (debounce) ด้วย 
  btnEnable.attach(pinSwEnable);
  btnEnable.interval(debounceDelay);

  //calcola range valori entro i quali considerare la posizione del joystick come "Stai fermo"
  //คำนวณช่วงค่าที่จะถูกพิจารณาว่าตำแหน่งของจอยสติกอยู่ในสถานะ "คงที่" โดยใช้ tresholdDown และ tresholdUp. สถานะ "คงที่" หมายถึงตำแหน่งที่จอยสติกอยู่ในส่วนกลางของช่วงค่าความเร็วของการเคลื่อนที่.
  tresholdDown = (maxSpeed / 2) - treshold;
  tresholdUp = (maxSpeed / 2) + treshold;

  //configura parametri dei motori
  motoreX.setMaxSpeed(maxSpeed);
  motoreX.setSpeed(minSpeed);
  motoreX.setAcceleration(accelerazione);

  motoreY.setMaxSpeed(maxSpeed);
  motoreY.setSpeed(minSpeed);
  motoreY.setAcceleration(accelerazione);
}

void loop() {

  //esegui funzione di controllo e lettura del bottone che determina lo stato di abilitazione
  checkEnable();

  digitalWrite(ledEnable, enable);  //mostra stato di abilitazione tramite il led su pin 13
  digitalWrite(pinEnable, !enable); //imposta valore opposto sui pin ENABLE dei driver

  //esegui lettura analogica dei valori provenienti dai potenziometri del joystick
  valX = analogRead(jX);
  valY = analogRead(jY);

  //mappa i valori letti in funzione della velocità inima e massima
  mapX = map(valX, 0, 1023, minSpeed, maxSpeed);
  mapY = map(valY, 0, 1023, minSpeed, maxSpeed);

  //esegui funzione di comando dei motori
  pilotaMotori(mapX, mapY);

}

//ฟังก์ชันที่ใช้ในการควบคุมการเคลื่อนไหวของมอเตอร์แกน X และ Y.
void pilotaMotori(long mapX, long mapY) {

  if (mapX <= tresholdDown) {
    //x va indietro
    speedX = -map(mapX, tresholdDown, minSpeed,   minSpeed, maxSpeed);
    muoviX = true;
  } else if (mapX >= tresholdUp) {
    //x va avanti
    speedX = map(mapX,  maxSpeed, tresholdUp,  maxSpeed, minSpeed);
    muoviX = true;
  } else {
    //x sta fermo
    speedX = 0;
    muoviX = false;
  }

  if (mapY <= tresholdDown) {
    //y va giù
    speedY = -map(mapY, tresholdDown, minSpeed,   minSpeed, maxSpeed);
    muoviY = true;
  } else if (mapY >= tresholdUp) {
    //y va su
    speedY = map(mapY,  maxSpeed, tresholdUp,  maxSpeed, minSpeed);
    muoviY = true;
  } else {
    //y sta fermo
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

//ฟังก์ชันที่ใช้ในการตรวจสอบสถานะของปุ่ม "เปิด/ปิด".
void checkEnable() {

  btnEnable.update();

  if (btnEnable.fell()) {
    enable = !enable;
  }

}