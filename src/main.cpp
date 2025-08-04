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


//CPUs a utilizar
#define PRO_CPU 0
#define APP_CPU 1
#define NOAFF_CPU tskNO_AFFINITY



// Crear objetos de motores
AccelStepper motor1(AccelStepper::FULL4WIRE, IN1_M1, IN3_M1, IN2_M1, IN4_M1);
AccelStepper motor2(AccelStepper::FULL4WIRE, IN1_M2, IN3_M2, IN2_M2, IN4_M2);


// Sensor LIDAR
VL53L0X sensor;

// Constantes
const int pasosPorGrado = 2048 / 360; // Motor 28BYJ-48 // Ajusta según pruebas
const int pasosPorMM = 50; // pasos para avanzar un mm // Ajusta según pruebas
const int margenSeguridad = 20; // mm, radio del robot


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


void TestHwm(char *taskName);
void TaskESCANEO(void *pvParameters);
void TaskROTARCOM(void *pvParameters);

//para sincronizacion
SemaphoreHandle_t xSemaphore = NULL;
volatile int tareasTerminadas = 0;  
//hay 2 tareas paralelas por loop, que gire 360 y que escanee
const int TOTAL_TAREAS = 2;


void setup() {
  Serial.begin(115200);
  // Inicializar EEPROM
  EEPROM.begin(512);

  //task paralelos
  xTaskCreatePinnedToCore(TaskESCANEO, "TaskESCANEO",4096,NULL,1,NULL,APP_CPU);
  xTaskCreatePinnedToCore(TaskROTARCOM, "TaskROTARCOM",4096,NULL,1,NULL,APP_CPU);

  // Intentar conectarse a la red guardada (la de la laptop/hotspot)
  // Si no puede, crea el AP para registrar la red
  iniciarConexionWiFi("ESP32_AP", "clave1234"); // Cambia nombre y clave si lo deseas

  // Configurar servidor web
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.begin();
  Serial.println("Servidor web iniciado");

  Wire.begin(21, 22); // SDA, SCL para VL53L0X

  sensor.init();
  sensor.setTimeout(500);
  sensor.startContinuous();

  motor1.setMaxSpeed(800);
  motor1.setAcceleration(400);

  motor2.setMaxSpeed(800);
  motor2.setAcceleration(400);

  //se inicia "semaforo"
  xSemaphore = xSemaphoreCreateBinary();
  //liberamos inicialmente
  xSemaphoreGive(xSemaphore); 
  delay(1000);
}

void loop() {
  // Manejar el servidor web
  server.handleClient();
  loopServidorWeb();
  // Escanear continuamente para mostrar el mapa del entorno

if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
    tareasTerminadas = 0;
    //se reinicia el contador por ciclo
    xSemaphoreGive(xSemaphore);
    
    xTaskCreatePinnedToCore(TaskESCANEO, "TaskESCANEO", 4096, NULL, 1, NULL, APP_CPU);
    xTaskCreatePinnedToCore(TaskROTARCOM, "TaskROTARCOM", 4096, NULL, 1, NULL, APP_CPU);
  }

  //AMBAS deben terminar
  while (true) {
    xSemaphoreTake(xSemaphore, portMAX_DELAY);
    if (tareasTerminadas >= TOTAL_TAREAS) {
      xSemaphoreGive(xSemaphore);
      break;
    }
    xSemaphoreGive(xSemaphore);
    delay(10);
    //seguridad
  }


  
  delay(3000); // Pausa entre escaneos (3 segundos)
  girarRobot(mejorAngulo);
  avanzarRobot((mayorDistancia)-margenSeguridad);
}

// ---------- FUNCIONES -------------

void escanearYBuscar() {
  mejorAngulo = 0;
  mayorDistancia = 0;

  // IDA: 0° a 360° (sentido antihorario/izquierda)
  for (int angulo = 0; angulo <= 360; angulo += 5) {
    
    

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
  delay(200); // escanea cada 200 milisegundos
  }
}

  void girarRobot(int angulo) {
  int pasosGiro = 6.516*map(angulo, 0, 360, 0, 2048);

  Serial.println("Girara " + String(angulo) + "° Tomara " + String(pasosGiro) + " pasos");

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
  int pasosAvance = 3.012*map(mm, 0, 360, 0, 2048);
  Serial.println("Debe avanzar "+ String(mm)+"mm Tomara " + String(pasosAvance) + " pasos");

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


void TestHwm(char *taskName){
  static int stack_hwm, stack_hwm_temp;

  stack_hwm_temp = uxTaskGetStackHighWaterMark(nullptr);
  if(!stack_hwm || (stack_hwm_temp < stack_hwm)){
    stack_hwm=stack_hwm_temp;
    Serial.printf("%s has stack hwm %u\n",taskName,stack_hwm);
  }
} 


void TaskESCANEO(void *pvParameters) {
  escanearYBuscar();
  
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  tareasTerminadas++;
  xSemaphoreGive(xSemaphore);
  
  vTaskDelete(NULL);
}

void TaskROTARCOM(void *pvParameters) {
  girarRobot(360);
  
  xSemaphoreTake(xSemaphore, portMAX_DELAY);
  tareasTerminadas++;
  xSemaphoreGive(xSemaphore);
  
  vTaskDelete(NULL);
}