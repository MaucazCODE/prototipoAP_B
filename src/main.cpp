#include <AccelStepper.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "apwifieeprommode.h"
#include <EEPROM.h>

// Definir el servidor web
WebServer server(80);

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
int historialAngulos[MAX_PUNTOS];
int historialDistancias[MAX_PUNTOS];
int numPuntos = 0;


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
float ultimoAngulo = 0;
float ultimaDistancia = 0;

// Posición del robot (en mm)
float robotX = 0;
float robotY = 0;
float robotAngulo = 0; // Ángulo actual del robot (0° = hacia adelante)

// Prototipos
void escanearYBuscar();
void girarRobot(int angulo);
void avanzarRobot(int mm);

void setup() {
  Serial.begin(115200);
  
  // Inicializar EEPROM
  EEPROM.begin(512);
  
  // Configurar directamente en modo AP (sin intentar conectar a Wi-Fi)
  Serial.println("Iniciando en modo Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("espgroup1", "12341243");
  
  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.begin();
  Serial.println("Servidor web iniciado en 192.168.4.1");
  
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
  // Manejar el servidor web
  server.handleClient();
  
  // Escanear continuamente para mostrar el mapa del entorno
  escanearYBuscar(); // Escanear para mostrar en la web
  delay(3000); // Pausa entre escaneos (3 segundos)
  girarRobot(mejorAngulo);
  avanzarRobot(mayorDistancia);
}

// ---------- FUNCIONES -------------

void escanearYBuscar() {
  mejorAngulo = 0;
  mayorDistancia = 0;
  numPuntos = 0;

  // Calibración del motor - ir a posición 0° con mayor precisión
  radarMotor.setCurrentPosition(0);
  radarMotor.setSpeed(500); // Velocidad más lenta para mayor precisión
  radarMotor.moveTo(0);
  while (radarMotor.distanceToGo() != 0) radarMotor.run();
  delay(200); // Pausa más larga para estabilizar

  // IDA: 0° a 360° (sentido antihorario/izquierda)
  for (int angulo = 0; angulo <= 360; angulo += 5) {
    int pasos = angulo * pasosPorGrado;
    radarMotor.moveTo(pasos);
    while (radarMotor.distanceToGo() != 0) radarMotor.run();
    delay(10); // escaneo más rápido

    int dist = sensor.readRangeContinuousMillimeters();
    Serial.print("→ Ángulo: "); Serial.print(angulo);
    Serial.print(" mm: "); Serial.println(dist);

    // Actualizar último escaneo
    ultimoAngulo = (float)angulo;
    ultimaDistancia = (float)dist;

    if (!sensor.timeoutOccurred() && dist < 2000) {
      if (numPuntos < MAX_PUNTOS) {
        historialAngulos[numPuntos] = angulo;
        historialDistancias[numPuntos] = dist;
        numPuntos++;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Mandado a la pagina ");
      server.send(200, "text/plain", "Mostrando dato");
    } else {
      Serial.println("No esta conectado el wifi");
    }

    if (!sensor.timeoutOccurred() && dist > mayorDistancia && dist < 2000) {
      mayorDistancia = dist;
      mejorAngulo = angulo;
    }
  }

  // REGRESO: 360° a 0° (sentido horario/derecha)
  for (int angulo = 360; angulo >= 0; angulo -= 5) {
    // Usamos el mismo cálculo de pasos que en la ida
    int pasos = angulo * pasosPorGrado;
    radarMotor.moveTo(pasos);
    while (radarMotor.distanceToGo() != 0) radarMotor.run();
    delay(10);

    int dist = sensor.readRangeContinuousMillimeters();
    Serial.print("← Ángulo: "); Serial.print(angulo);
    Serial.print(" mm: "); Serial.println(dist);

    // Actualizar último escaneo
    ultimoAngulo = (float)angulo;
    ultimaDistancia = (float)dist;

    if (!sensor.timeoutOccurred() && dist < 2000) {
      if (numPuntos < MAX_PUNTOS) {
        historialAngulos[numPuntos] = angulo;
        historialDistancias[numPuntos] = dist;
        numPuntos++;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Mandado a la pagina ");
      server.send(200, "text/plain", "Mostrando dato");
    } else {
      Serial.println("No esta conectado el wifi");
    }

    if (!sensor.timeoutOccurred() && dist > mayorDistancia && dist < 2000) {
      mayorDistancia = dist;
      mejorAngulo = angulo;
    }
  }

  // Verificación final en posición 0°
  radarMotor.setSpeed(500); // Velocidad más lenta para mayor precisión
  radarMotor.moveTo(0);
  while (radarMotor.distanceToGo() != 0) radarMotor.run();
  delay(200); // Pausa más larga para estabilizar

  // Restaurar velocidad normal
  radarMotor.setSpeed(2000);
}

  void girarRobot(int angulo) {
  int pasosGiro = map(angulo, 0, 180, 0, 1024);

  motor1.moveTo(motor1.currentPosition() - pasosGiro); // Izquierda atrás
  motor2.moveTo(motor2.currentPosition() + pasosGiro); // Derecha adelante

  while (motor1.distanceToGo() != 0 || motor2.distanceToGo() != 0) {
    motor1.run();
    motor2.run();
    }
    
  // Actualizar ángulo del robot
  robotAngulo += angulo;
  if (robotAngulo >= 360) robotAngulo -= 360;
  if (robotAngulo < 0) robotAngulo += 360;
  
  Serial.println("Robot giró " + String(angulo) + "°. Ángulo actual: " + String(robotAngulo) + "°");
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
    
  // Actualizar posición del robot
  float radianes = robotAngulo * 3.14159265 / 180.0;
  robotX += mm * cos(radianes);
  robotY += mm * sin(radianes);
  
  Serial.println("Salio del loop de avnce ");
  Serial.println("Posición robot: X=" + String(robotX) + " Y=" + String(robotY) + " Ángulo=" + String(robotAngulo));
}