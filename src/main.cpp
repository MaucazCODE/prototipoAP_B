#include <AccelStepper.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "apwifieeprommode.h"
#include <EEPROM.h>

// Pines motor 1 (izquierdo)
#define IN1_M1 14
#define IN2_M1 27
#define IN3_M1 26
#define IN4_M1 25

// Pines motor 2 (derecho)
#define IN1_M2 19
#define IN2_M2 18
#define IN3_M2 5
#define IN4_M2 32

// Pines motor 3 (radar)
#define IN1_M3 23
#define IN2_M3 33
#define IN3_M3 4
#define IN4_M3 2


//definir puntos
#define MAX_PUNTOS 100
volatile int historialAngulos[MAX_PUNTOS];
volatile int historialDistancias[MAX_PUNTOS];
volatile int numPuntos = 0;


// Crear objetos de motores
AccelStepper motor1(AccelStepper::FULL4WIRE, IN1_M1, IN3_M1, IN2_M1, IN4_M1);
AccelStepper motor2(AccelStepper::FULL4WIRE, IN1_M2, IN3_M2, IN2_M2, IN4_M2);
AccelStepper radarMotor(AccelStepper::FULL4WIRE, IN1_M3, IN3_M3, IN2_M3, IN4_M3);

// Sensor LIDAR
VL53L0X sensor;

// Constantes
const int pasosPorGrado = 2048 / 360; // Motor 28BYJ-48 // Ajusta según pruebas
const int pasosPorMM = 50; // pasos para avanzar un mm // Ajusta según pruebas
const int margenSeguridad = 100; // mm, radio del robot


// Variables
int mejorAngulo = 0;
int mayorDistancia = 0;

// Prototipos
void escanearYBuscar();
void girarRobot(int angulo);
void avanzarRobot(int mm);

void setup() {
  Serial.begin(115200);
  intentoconexion("espgroup1", "12341243");
  Wire.begin(21, 22); // SDA, SCL para VL53L0X

  sensor.init();
  sensor.setTimeout(500);
  sensor.startContinuous();

  motor1.setMaxSpeed(800);
  motor1.setAcceleration(400);

  motor2.setMaxSpeed(800);
  motor2.setAcceleration(400);

  radarMotor.setMaxSpeed(2000); // MÁS RÁPIDO
  radarMotor.setAcceleration(2000); // MÁS RÁPIDO

  delay(1000);
}

void loop() {
loopAP();
escanearYBuscar(); // 1. Escanea
girarRobot(mejorAngulo); // 2. Gira al mejor ángulo
avanzarRobot(mayorDistancia - margenSeguridad); // 3. Avanza con margen
delay(1000); // Pausa antes de repetir ciclo
}

// ---------- FUNCIONES -------------

void escanearYBuscar() {
mejorAngulo = 0;
mayorDistancia = 0;
numPuntos = 0;

// IDA: 0° a 360°
for (int angulo = 0; angulo <= 360; angulo += 5) {
  int pasos = angulo * pasosPorGrado;
  radarMotor.moveTo(pasos);
  while (radarMotor.distanceToGo() != 0) radarMotor.run();
  delay(10); // escaneo más rápido

  int dist = sensor.readRangeContinuousMillimeters();
  Serial.print("→ Ángulo: "); Serial.print(angulo);
  Serial.print(" mm: "); Serial.println(dist);

if (!sensor.timeoutOccurred() && dist < 2000) {
    if (numPuntos < MAX_PUNTOS) {
        historialAngulos[numPuntos] = angulo;
        historialDistancias[numPuntos] = dist;
        numPuntos++;
    }
  }



  if (WiFi.status() ==WL_CONNECTED){
    Serial.println("Mandado a la pagina ");
    server.send(200, "text/plain", "Mostrando dato");
  } else{
  Serial.println("No esta conectado el wifi"); } 

  if (!sensor.timeoutOccurred() && dist > mayorDistancia && dist < 2000) {
  mayorDistancia = dist;
  mejorAngulo = angulo;
  }
}

// REGRESO: 360° a 0°
for (int angulo = 360; angulo >= 0; angulo -= 5) {
  int pasos = angulo * pasosPorGrado*3;
  radarMotor.moveTo(pasos);
  while (radarMotor.distanceToGo() != 0) radarMotor.run();
  delay(10);

  int dist = sensor.readRangeContinuousMillimeters();
  Serial.print("← Ángulo: "); Serial.print(angulo);
  Serial.print(" mm: "); Serial.println(dist);


if (!sensor.timeoutOccurred() && dist < 2000) {
    if (numPuntos < MAX_PUNTOS) {
        historialAngulos[numPuntos] = angulo;
        historialDistancias[numPuntos] = dist;
        numPuntos++;
     }
 }






  if (WiFi.status() ==WL_CONNECTED){
    Serial.println("Mandado a la pagina ");
    server.send(200, "text/plain", "Mostrando dato");
  } else{
  Serial.println("No esta conectado el wifi"); } 

  if (!sensor.timeoutOccurred() && dist > mayorDistancia && dist < 2000) {
    mayorDistancia = dist;
    mejorAngulo = angulo;
    }
}

// Regresa a posición 0°
radarMotor.moveTo(0);
while (radarMotor.distanceToGo() != 0) radarMotor.run();
}

  void girarRobot(int angulo) {
  int pasosGiro = map(angulo, 0, 180, 0, 1024);

  motor1.moveTo(motor1.currentPosition() - pasosGiro); // Izquierda atrás
  motor2.moveTo(motor2.currentPosition() + pasosGiro); // Derecha adelante

  while (motor1.distanceToGo() != 0 || motor2.distanceToGo() != 0) {
    motor1.run();
    motor2.run();
    }
}

void avanzarRobot(int mm) {
  if (mm <= 0) {return;}
  int pasosAvance = map(mm, 0, 300, 0, 2048);
  Serial.println("Debe avanzar "+ String(pasosAvance));

  motor1.moveTo(motor1.currentPosition() + pasosAvance);
  motor2.moveTo(motor2.currentPosition() + pasosAvance);

  while (motor1.distanceToGo() != 0 && motor2.distanceToGo() != 0) {
    motor1.run();
    motor2.run();
    }
  Serial.println("Salio del loop de avnce ");
}