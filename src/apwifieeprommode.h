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

// --- Endpoint para obtener datos actualizados (JSON) ---
void handleGetData() {
    String json = "{";
    json += "\"numPuntos\":" + String(numPuntos) + ",";
    json += "\"ultimoAngulo\":" + String(ultimoAngulo, 1) + ",";
    json += "\"ultimaDistancia\":" + String(ultimaDistancia, 1) + ",";
    json += "\"robotX\":" + String(robotX, 1) + ",";
    json += "\"robotY\":" + String(robotY, 1) + ",";
    json += "\"robotAngulo\":" + String(robotAngulo, 1) + ",";
    
    // Generar arrays de puntos en coordenadas del mundo real
    json += "\"obstaculos\":[";
    for (int i = 0; i < numPuntos; i++) {
        // Calcular posición absoluta del obstáculo en coordenadas del mundo
        float anguloAbsoluto = historialAngulos[i] + robotAngulo;
        float anguloRad = anguloAbsoluto * 3.14159265 / 180.0;
        float obstaculoX = robotX + historialDistancias[i] * cos(anguloRad);
        float obstaculoY = robotY + historialDistancias[i] * sin(anguloRad);
        
        json += "{\"x\":" + String(obstaculoX, 1) + ",\"y\":" + String(obstaculoY, 1) + "}";
        if (i < numPuntos - 1) json += ",";
    }
    json += "]}";
    
    server.send(200, "application/json", json);
}

// --- Página principal (Mapa Cartesiano) ---
void handleRoot() {
    int canvasSize = 600; // Aumentamos el tamaño para mejor visualización
    
    // Calcular coordenadas iniciales para el mapa
    String obstaculos = "[";
    for (int i = 0; i < numPuntos; i++) {
        float anguloAbsoluto = historialAngulos[i] + robotAngulo;
        float anguloRad = anguloAbsoluto * 3.14159265 / 180.0;
        float obstaculoX = robotX + historialDistancias[i] * cos(anguloRad);
        float obstaculoY = robotY + historialDistancias[i] * sin(anguloRad);
        
        obstaculos += "{\"x\":" + String(obstaculoX, 1) + ",\"y\":" + String(obstaculoY, 1) + "}";
        if (i < numPuntos - 1) obstaculos += ",";
    }
    obstaculos += "]";

    // HTML con mapa cartesiano
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<title>Mapa Robot ESP32</title>";
    html += "<style>";
    html += "body { font-family: Arial; background: #0a0a0a; color: #eee; text-align: center; padding: 20px; margin: 0; }";
    html += ".container { max-width: 1200px; margin: 0 auto; }";
    html += ".header { background: #1a1a1a; border-radius: 10px; padding: 20px; margin-bottom: 20px; }";
    html += "h1 { color: #00ff88; margin: 0 0 10px 0; font-size: 2.5em; }";
    html += ".status { display: inline-block; padding: 8px 15px; border-radius: 20px; font-weight: bold; margin-left: 15px; }";
    html += ".map-container { background: #1a1a1a; border-radius: 15px; padding: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }";
    html += "#mapaCanvas { background: #000; border: 2px solid #00ff88; border-radius: 10px; }";
    html += ".info-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin-top: 20px; }";
    html += ".info-card { background: #2a2a2a; border-radius: 10px; padding: 15px; }";
    html += ".info-title { color: #00ff88; font-weight: bold; margin-bottom: 10px; }";
    html += ".info-value { font-size: 1.3em; color: #fff; }";
    html += ".legend { display: flex; justify-content: center; gap: 30px; margin-top: 15px; flex-wrap: wrap; }";
    html += ".legend-item { display: flex; align-items: center; gap: 8px; }";
    html += ".legend-color { width: 15px; height: 15px; border-radius: 50%; border: 2px solid #fff; }";
    html += "</style></head><body>";

    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<h1>Mapa del Entorno</h1>";
    html += "<span class='status' id='status' style='background: #28a745;'> En línea</span>";
    html += "</div>";

    html += "<div class='map-container'>";
    html += "<canvas id='mapaCanvas' width='" + String(canvasSize) + "' height='" + String(canvasSize) + "'></canvas>";
    
    html += "<div class='legend'>";
    html += "<div class='legend-item'><div class='legend-color' style='background: #00ff00;'></div><span>Robot</span></div>";
    html += "<div class='legend-item'><div class='legend-color' style='background: #ff0040;'></div><span>Obstáculos</span></div>";
    html += "<div class='legend-item'><div class='legend-color' style='background: #0088ff;'></div><span>Trayectoria</span></div>";
    html += "<div class='legend-item'><div class='legend-color' style='background: #ffaa00;'></div><span>Dirección</span></div>";
    html += "</div>";
    html += "</div>";

    html += "<div class='info-grid'>";
    html += "<div class='info-card'>";
    html += "<div class='info-title'>📡 Último Escaneo</div>";
    html += "<div class='info-value'><span id='ultimoAngulo'>" + String(ultimoAngulo, 1) + "</span>° - <span id='ultimaDistancia'>" + String(ultimaDistancia, 1) + "</span> mm</div>";
    html += "</div>";
    html += "<div class='info-card'>";
    html += "<div class='info-title'> Obstáculos Detectados</div>";
    html += "<div class='info-value'><span id='numPuntos'>" + String(numPuntos) + "</span></div>";
    html += "</div>";
    html += "<div class='info-card'>";
    html += "<div class='info-title'>Posición Robot</div>";
    html += "<div class='info-value'>X: <span id='robotX'>" + String(robotX, 1) + "</span> mm</div>";
    html += "<div class='info-value'>Y: <span id='robotY'>" + String(robotY, 1) + "</span> mm</div>";
    html += "</div>";
    html += "<div class='info-card'>";
    html += "<div class='info-title'> Orientación</div>";
    html += "<div class='info-value'><span id='robotAngulo'>" + String(robotAngulo, 1) + "</span>°</div>";
    html += "</div>";
    html += "</div>";
    html += "</div>";

    // JavaScript para mapa cartesiano en tiempo real
    html += "<script>";
    html += "let canvas = document.getElementById('mapaCanvas');";
    html += "let ctx = canvas.getContext('2d');";
    html += "let canvasSize = " + String(canvasSize) + ";";
    html += "let ultimoNumPuntos = " + String(numPuntos) + ";";
    html += "let trayectoriaRobot = [{x: " + String(robotX) + ", y: " + String(robotY) + "}];";
    html += "let escalaPixelPorMM = 0.15;"; // Escala: 0.15 pixels por mm
    html += "let offsetX = 0, offsetY = 0;"; // Para centrar el mapa dinámicamente

    // Función para convertir coordenadas del mundo a canvas
    html += "function mundoACanvas(x, y) {";
    html += "  return {";
    html += "    x: (canvasSize / 2) + (x * escalaPixelPorMM) + offsetX,";
    html += "    y: (canvasSize / 2) - (y * escalaPixelPorMM) + offsetY"; // Y invertida para que arriba sea positivo
    html += "  };";
    html += "}";

    // Función para dibujar grilla
    html += "function dibujarGrilla() {";
    html += "  ctx.strokeStyle = '#333';";
    html += "  ctx.lineWidth = 1;";
    html += "  let gridSize = 500 * escalaPixelPorMM;"; // Líneas cada 500mm
    html += "  for(let i = -canvasSize; i <= canvasSize * 2; i += gridSize) {";
    html += "    ctx.beginPath();";
    html += "    ctx.moveTo(i + offsetX, 0);";
    html += "    ctx.lineTo(i + offsetX, canvasSize);";
    html += "    ctx.stroke();";
    html += "    ctx.beginPath();";
    html += "    ctx.moveTo(0, i + offsetY);";
    html += "    ctx.lineTo(canvasSize, i + offsetY);";
    html += "    ctx.stroke();";
    html += "  }";
    // Ejes principales
    html += "  ctx.strokeStyle = '#555';";
    html += "  ctx.lineWidth = 2;";
    html += "  ctx.beginPath();";
    html += "  ctx.moveTo(canvasSize/2 + offsetX, 0);";
    html += "  ctx.lineTo(canvasSize/2 + offsetX, canvasSize);";
    html += "  ctx.stroke();";
    html += "  ctx.beginPath();";
    html += "  ctx.moveTo(0, canvasSize/2 + offsetY);";
    html += "  ctx.lineTo(canvasSize, canvasSize/2 + offsetY);";
    html += "  ctx.stroke();";
    html += "}";

    // Función para dibujar el mapa completo
    html += "function dibujarMapa(robotX, robotY, robotAngulo, obstaculos) {";
    html += "  ctx.clearRect(0, 0, canvasSize, canvasSize);";
    html += "  ";
    html += "  dibujarGrilla();";
    html += "  ";
    html += "  let posRobot = mundoACanvas(robotX, robotY);";
    html += "  ";
    // Dibujar trayectoria del robot
    html += "  if(trayectoriaRobot.length > 1) {";
    html += "    ctx.strokeStyle = '#0088ff';";
    html += "    ctx.lineWidth = 3;";
    html += "    ctx.beginPath();";
    html += "    let primerPunto = mundoACanvas(trayectoriaRobot[0].x, trayectoriaRobot[0].y);";
    html += "    ctx.moveTo(primerPunto.x, primerPunto.y);";
    html += "    for(let i = 1; i < trayectoriaRobot.length; i++) {";
    html += "      let punto = mundoACanvas(trayectoriaRobot[i].x, trayectoriaRobot[i].y);";
    html += "      ctx.lineTo(punto.x, punto.y);";
    html += "    }";
    html += "    ctx.stroke();";
    html += "  }";
    html += "  ";
    // Dibujar obstáculos
    html += "  obstaculos.forEach(obstaculo => {";
    html += "    let pos = mundoACanvas(obstaculo.x, obstaculo.y);";
    html += "    ctx.beginPath();";
    html += "    ctx.arc(pos.x, pos.y, 4, 0, 2 * Math.PI);";
    html += "    ctx.fillStyle = '#ff0040';";
    html += "    ctx.fill();";
    html += "    ctx.strokeStyle = '#ff4070';";
    html += "    ctx.lineWidth = 2;";
    html += "    ctx.stroke();";
    html += "  });";
    html += "  ";
    // Dibujar robot con orientación
    html += "  ctx.beginPath();";
    html += "  ctx.arc(posRobot.x, posRobot.y, 8, 0, 2 * Math.PI);";
    html += "  ctx.fillStyle = '#00ff00';";
    html += "  ctx.fill();";
    html += "  ctx.strokeStyle = '#00cc00';";
    html += "  ctx.lineWidth = 3;";
    html += "  ctx.stroke();";
    html += "  ";
    // Flecha de dirección del robot
    html += "  let anguloRad = robotAngulo * Math.PI / 180;";
    html += "  let flechaX = posRobot.x + Math.cos(anguloRad) * 20;";
    html += "  let flechaY = posRobot.y - Math.sin(anguloRad) * 20;";
    html += "  ctx.beginPath();";
    html += "  ctx.moveTo(posRobot.x, posRobot.y);";
    html += "  ctx.lineTo(flechaX, flechaY);";
    html += "  ctx.strokeStyle = '#ffaa00';";
    html += "  ctx.lineWidth = 4;";
    html += "  ctx.stroke();";
    html += "  ";
    // Punta de flecha
    html += "  let punta1X = flechaX - Math.cos(anguloRad - 0.5) * 8;";
    html += "  let punta1Y = flechaY + Math.sin(anguloRad - 0.5) * 8;";
    html += "  let punta2X = flechaX - Math.cos(anguloRad + 0.5) * 8;";
    html += "  let punta2Y = flechaY + Math.sin(anguloRad + 0.5) * 8;";
    html += "  ctx.beginPath();";
    html += "  ctx.moveTo(flechaX, flechaY);";
    html += "  ctx.lineTo(punta1X, punta1Y);";
    html += "  ctx.moveTo(flechaX, flechaY);";
    html += "  ctx.lineTo(punta2X, punta2Y);";
    html += "  ctx.stroke();";
    html += "}";

    // Función para actualizar todos los datos sin recargar
    html += "function actualizarDatos() {";
    html += "  fetch('/get-data')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('numPuntos').textContent = data.numPuntos;";
    html += "      document.getElementById('ultimoAngulo').textContent = data.ultimoAngulo;";
    html += "      document.getElementById('ultimaDistancia').textContent = data.ultimaDistancia;";
    html += "      document.getElementById('robotX').textContent = data.robotX;";
    html += "      document.getElementById('robotY').textContent = data.robotY;";
    html += "      document.getElementById('robotAngulo').textContent = data.robotAngulo;";
    html += "      ";
    // Actualizar trayectoria si el robot se movió
    html += "      let ultimaPosicion = trayectoriaRobot[trayectoriaRobot.length - 1];";
    html += "      if(Math.abs(ultimaPosicion.x - data.robotX) > 10 || Math.abs(ultimaPosicion.y - data.robotY) > 10) {";
    html += "        trayectoriaRobot.push({x: data.robotX, y: data.robotY});";
    html += "        if(trayectoriaRobot.length > 50) trayectoriaRobot.shift();"; // Limitar trayectoria
    html += "      }";
    html += "      ";
    html += "      dibujarMapa(data.robotX, data.robotY, data.robotAngulo, data.obstaculos);";
    html += "      document.getElementById('status').innerHTML = '🟢 En línea';";
    html += "      if(data.numPuntos !== ultimoNumPuntos) {";
    html += "        ultimoNumPuntos = data.numPuntos;";
    html += "        console.log('Nuevos obstáculos detectados: ' + data.numPuntos);";
    html += "      }";
    html += "    })";
    html += "    .catch(error => {";
    html += "      document.getElementById('status').innerHTML = '🔴 Desconectado';";
    html += "      document.getElementById('status').style.background = '#dc3545';";
    html += "      console.error('Error:', error);";
    html += "    });";
    html += "}";

    // Dibujar mapa inicial
    html += "dibujarMapa(" + String(robotX) + ", " + String(robotY) + ", " + String(robotAngulo) + ", " + obstaculos + ");";
    
    // Actualizar cada 1.5 segundos
    html += "setInterval(actualizarDatos, 1500);";
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
    server.on("/get-data", handleGetData);
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
        server.on("/get-data", handleGetData);
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