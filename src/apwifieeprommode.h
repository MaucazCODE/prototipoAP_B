#ifndef AP_WIFI_EEPROM_MODE_H
#define AP_WIFI_EEPROM_MODE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <Wire.h>
#include "WiFi.h"

// Variables externas
extern int numPuntos;
extern int historialAngulos[];
extern int historialDistancias[];
extern float ultimoAngulo;
extern float ultimaDistancia;
extern float robotX;
extern float robotY;
extern float robotAngulo;

extern WebServer server;

// Variable global para el último número de puntos enviado
int ultimoNumPuntosEnviado = 0;

// --- Utilidades EEPROM ---
String leerStringDeEEPROM(int direccion) {
    String cadena = "";
    char caracter = EEPROM.read(direccion);
    int i = 0;
    while (caracter != '\0' && i < 100) {
        cadena += caracter;
        i++;
        caracter = EEPROM.read(direccion + i);
    }
    return cadena;
}

void escribirStringEnEEPROM(int direccion, String cadena) {
    int longitudCadena = cadena.length();
    for (int i = 0; i < longitudCadena; i++) {
        EEPROM.write(direccion + i, cadena[i]);
    }
    EEPROM.write(direccion + longitudCadena, '\0');
    EEPROM.commit();
}

// --- Endpoint para verificar nuevos puntos (AJAX) ---
void handleCheckUpdates() {
    server.send(200, "text/plain", String(numPuntos));
}

// --- Página principal (Solo Radar) ---
void handleRoot() {
    int radioMax = 2000; // mm
    int canvasSize = 400; // px
    int center = canvasSize / 2;
    float escala = (float)center / radioMax;

    // Generar coordenadas de TODOS los puntos acumulados
    String puntosX = "[";
    String puntosY = "[";
    for (int i = 0; i < numPuntos; i++) {
        float anguloAbsoluto = historialAngulos[i] + robotAngulo;
        float anguloRad = anguloAbsoluto * 3.14159265 / 180.0;
        float puntoX = robotX + historialDistancias[i] * cos(anguloRad);
        float puntoY = robotY + historialDistancias[i] * sin(anguloRad);
        int x = center + int(puntoX * escala);
        int y = center - int(puntoY * escala);
        puntosX += String(x);
        puntosY += String(y);
        if (i < numPuntos - 1) {
            puntosX += ",";
            puntosY += ",";
        }
    }
    puntosX += "]";
    puntosY += "]";

    // HTML con actualización automática vía AJAX
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<title>Radar Robot ESP32</title>";
    html += "<style>";
    html += "body { font-family: Arial; background: #181a1b; color: #eee; text-align: center; padding: 40px; margin: 0; }";
    html += ".card { background: #222; border-radius: 10px; box-shadow: 0 2px 8px rgba(0,0,0,0.3); display: inline-block; padding: 30px 50px; }";
    html += "h1 { color: #3399ff; margin-bottom: 30px; }";
    html += ".dato { font-size: 2em; margin: 20px 0; }";
    html += ".info { font-size: 1.2em; margin: 15px 0; color: #aaa; }";
    html += ".status { position: absolute; top: 10px; right: 10px; padding: 5px 10px; background: #333; border-radius: 5px; font-size: 0.9em; }";
    html += ".online { background: #28a745; }";
    html += ".scanning { background: #ffc107; color: #000; }";
    html += "</style></head><body>";

    html += "<div class='status' id='status'>Conectado</div>";
    html += "<div class='card'>";
    html += "<h1> Mapa del Entorno</h1>";
    html += "<canvas id='radarCanvas' width='" + String(canvasSize) + "' height='" + String(canvasSize) + "' style='background:#000; border-radius:50%; border: 2px solid #3399ff;'></canvas>";
    html += "<div class='dato'>Obstáculos detectados: <span id='numPuntos'>" + String(numPuntos) + "</span></div>";
    html += "<div class='info'>";
    html += "<p> Último escaneo: <span id='ultimoAngulo'>" + String(ultimoAngulo, 1) + "</span>° - <span id='ultimaDistancia'>" + String(ultimaDistancia, 1) + "</span> mm</p>";
    html += "<p> Posición: X=<span id='robotX'>" + String(robotX, 1) + "</span> Y=<span id='robotY'>" + String(robotY, 1) + "</span> Ángulo=<span id='robotAngulo'>" + String(robotAngulo, 1) + "</span>°</p>";
    html += "</div>";
    html += "</div>";

    // JavaScript para actualización automática y dibujo del radar
    html += "<script>";
    html += "let ultimoNumPuntos = " + String(numPuntos) + ";";
    html += "let canvas = document.getElementById('radarCanvas');";
    html += "let ctx = canvas.getContext('2d');";
    html += "let canvasSize = " + String(canvasSize) + ";";
    html += "let center = " + String(center) + ";";
    
    // Función para dibujar el radar
    html += "function dibujarRadar(puntosX, puntosY) {";
    html += "  ctx.clearRect(0, 0, canvasSize, canvasSize);";
    // Círculos concéntricos
    html += "  for(let r = 50; r < center; r += 50) {";
    html += "    ctx.beginPath();";
    html += "    ctx.arc(center, center, r, 0, 2 * Math.PI);";
    html += "    ctx.strokeStyle = '#333';";
    html += "    ctx.lineWidth = 1;";
    html += "    ctx.stroke();";
    html += "  }";
    // Líneas radiales
    html += "  for(let a = 0; a < 360; a += 45) {";
    html += "    let rad = a * Math.PI / 180;";
    html += "    let x = center + Math.cos(rad) * (center - 10);";
    html += "    let y = center + Math.sin(rad) * (center - 10);";
    html += "    ctx.beginPath();";
    html += "    ctx.moveTo(center, center);";
    html += "    ctx.lineTo(x, y);";
    html += "    ctx.strokeStyle = '#333';";
    html += "    ctx.lineWidth = 1;";
    html += "    ctx.stroke();";
    html += "  }";
    // Borde exterior
    html += "  ctx.beginPath();";
    html += "  ctx.arc(center, center, center - 5, 0, 2 * Math.PI);";
    html += "  ctx.strokeStyle = '#3399ff';";
    html += "  ctx.lineWidth = 2;";
    html += "  ctx.stroke();";
    // Robot (centro)
    html += "  ctx.beginPath();";
    html += "  ctx.arc(center, center, 6, 0, 2 * Math.PI);";
    html += "  ctx.fillStyle = '#00ff00';";
    html += "  ctx.fill();";
    html += "  ctx.strokeStyle = '#00cc00';";
    html += "  ctx.lineWidth = 2;";
    html += "  ctx.stroke();";
    // Puntos detectados
    html += "  for(let i = 0; i < puntosX.length; i++) {";
    html += "    ctx.beginPath();";
    html += "    ctx.arc(puntosX[i], puntosY[i], 3, 0, 2 * Math.PI);";
    html += "    ctx.fillStyle = '#ff0000';";
    html += "    ctx.fill();";
    html += "    ctx.strokeStyle = '#cc0000';";
    html += "    ctx.lineWidth = 1;";
    html += "    ctx.stroke();";
    html += "  }";
    html += "}";

    // Función para actualizar datos
    html += "function actualizarDatos() {";
    html += "  fetch('/check-updates')";
    html += "    .then(response => response.text())";
    html += "    .then(numPuntos => {";
    html += "      if(parseInt(numPuntos) !== ultimoNumPuntos) {";
    html += "        document.getElementById('status').className = 'status scanning';";
    html += "        document.getElementById('status').textContent = 'Actualizando...';";
    html += "        setTimeout(() => location.reload(), 500);";
    html += "      } else {";
    html += "        document.getElementById('status').className = 'status online';";
    html += "        document.getElementById('status').textContent = 'En línea';";
    html += "      }";
    html += "    })";
    html += "    .catch(() => {";
    html += "      document.getElementById('status').className = 'status';";
    html += "      document.getElementById('status').textContent = 'Desconectado';";
    html += "    });";
    html += "}";

    // Dibujar radar inicial
    html += "dibujarRadar(" + puntosX + ", " + puntosY + ");";
    
    // Verificar actualizaciones cada 2 segundos
    html += "setInterval(actualizarDatos, 2000);";
    html += "</script>";

    html += "</body></html>";
    server.send(200, "text/html", html);
}

// --- Manejo de registro WiFi ---
void handleWifi() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    escribirStringEnEEPROM(0, ssid);
    escribirStringEnEEPROM(100, password);
    server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='2;url=/'></head><body style='background:#181a1b; color:#fff; text-align:center; padding-top:100px;'><h1>Red guardada. Reiniciando...</h1></body></html>");
    delay(2000);
    ESP.restart();
}

// --- Intenta conectar a la última red guardada ---
bool conectarUltimaRed() {
    String ssid = leerStringDeEEPROM(0);
    String password = leerStringDeEEPROM(100);
    if (ssid.length() == 0) return false;
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());
    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED && cnt < 10) {
        delay(1000);
        cnt++;
    }
    return WiFi.status() == WL_CONNECTED;
}

// --- Inicia Access Point y servidor web ---
void iniciarAP(const char* apSsid, const char* apPassword) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);
    server.on("/", handleRoot);
    server.on("/check-updates", handleCheckUpdates);
    server.on("/wifi", HTTP_POST, handleWifi);
    server.begin();
    Serial.println("Servidor web iniciado en modo AP.");
}

// --- Lógica principal de conexión WiFi ---
void iniciarConexionWiFi(const char* apSsid, const char* apPassword) {
    EEPROM.begin(512);
    if (conectarUltimaRed()) {
        Serial.println("Conectado a la red guardada.");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        server.on("/", handleRoot);
        server.on("/check-updates", handleCheckUpdates);
        server.on("/wifi", HTTP_POST, handleWifi);
        server.begin();
    } else {
        Serial.println("No se pudo conectar. Iniciando Access Point para registrar nueva red.");
        iniciarAP(apSsid, apPassword);
    }
}

// --- Loop para servidor web ---
void loopServidorWeb() {
    server.handleClient();
}

#endif // AP_WIFI_EEPROM_MODE_H