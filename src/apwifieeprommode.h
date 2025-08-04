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

// --- Página principal (Radar + WiFi) ---
void handleRoot() {
    int radioMax = 2000; // mm
    int canvasSize = 400; // px
    int center = canvasSize / 2;
    float escala = (float)center / radioMax;

    String tab = server.hasArg("tab") ? server.arg("tab") : "wifi";
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

    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<title>Radar Robot ESP32</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background: #181a1b; color: #eee; text-align: center; padding: 40px; }";
    html += ".card { background: #222; border-radius: 10px; box-shadow: 0 2px 8px rgba(0,0,0,0.3); display: inline-block; padding: 30px 50px; }";
    html += "h1 { color: #3399ff; }";
    html += ".dato { font-size: 2em; margin: 20px 0; }";
    html += ".info { font-size: 1.2em; margin: 15px 0; color: #aaa; }";
    html += ".tab { display: inline-block; padding: 10px 30px; margin: 0 5px; background: #333; border-radius: 10px 10px 0 0; cursor: pointer; font-weight: bold; color: #aaa; }";
    html += ".tab.active { background: #222; border-bottom: 2px solid #222; color: #3399ff; }";
    html += ".tab-content { display: none; padding: 30px 0; }";
    html += ".tab-content.active { display: block; }";
    html += "form { margin: 30px 0; } input[type=text], input[type=password] { padding: 10px; width: 220px; margin: 10px 0; border-radius: 5px; border: 1px solid #444; background: #181a1b; color: #eee; } input[type=submit] { padding: 10px 30px; border-radius: 5px; background: #3399ff; color: #fff; border: none; font-size: 1em; cursor: pointer; } input[type=submit]:hover { background: #1976d2; }";
    html += "</style></head><body>";
    html += "<div style='margin-bottom: 0px;'>";
    html += "<span class='tab" + String(tab == "wifi" ? " active" : "") + "' onclick=\"showTab('wifi')\">Configurar WiFi</span>";
    html += "<span class='tab" + String(tab == "radar" ? " active" : "") + "' onclick=\"showTab('radar')\">Radar</span>";
    html += "</div>";
    html += "<div class='card'>";
    // Tab WiFi
    html += "<div id='wifi' class='tab-content" + String(tab == "wifi" ? " active" : "") + "'>";
    html += "<h1>Configurar WiFi</h1>";
    html += "<form action='/wifi' method='post'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "Password: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Registrar'>";
    html += "</form>";
    html += "</div>";
    // Tab Radar
    html += "<div id='radar' class='tab-content" + String(tab == "radar" ? " active" : "") + "'>";
    html += "<h1>Mapa del Entorno</h1>";
    html += "<canvas id='radarCanvas' width='" + String(canvasSize) + "' height='" + String(canvasSize) + "' style='background:#000000; border-radius:50%;'></canvas>";
    html += "<div class='dato'>Obstáculos detectados: <b>" + String(numPuntos) + "</b></div>";
    html += "<div class='info'><p>Último escaneo: " + String(ultimoAngulo, 1) + "° - " + String(ultimaDistancia, 1) + " mm</p>";
    html += "<p>Posición robot: X=" + String(robotX, 1) + " Y=" + String(robotY, 1) + " Ángulo=" + String(robotAngulo, 1) + "°</p></div>";
    html += "</div>";
    html += "</div>";
    html += "<script>";
    html += "function showTab(tab) {";
    html += "  document.getElementById('wifi').classList.remove('active');";
    html += "  document.getElementById('radar').classList.remove('active');";
    html += "  var tabs = document.getElementsByClassName('tab');";
    html += "  for (var i = 0; i < tabs.length; i++) tabs[i].classList.remove('active');";
    html += "  document.getElementById(tab).classList.add('active');";
    html += "  if(tab==='wifi') tabs[0].classList.add('active'); else tabs[1].classList.add('active');";
    html += "  if(history.pushState) { history.pushState(null, null, '/?tab=' + tab); }";
    html += "}";
    html += "document.addEventListener('DOMContentLoaded', function() { drawRadar(); });";
    html += "function drawRadar() {";
    html += "var c = document.getElementById('radarCanvas'); if(!c) return; var ctx = c.getContext('2d'); ctx.clearRect(0,0," + String(canvasSize) + "," + String(canvasSize) + ");";
    html += "for(var r=50;r<" + String(center) + ";r+=50){ctx.beginPath();ctx.arc(" + String(center) + "," + String(center) + ",r,0,2*Math.PI);ctx.strokeStyle='#333333';ctx.lineWidth=1;ctx.stroke();}";
    html += "for(var a=0;a<360;a+=45){var rad = a * Math.PI / 180;var x = " + String(center) + " + Math.cos(rad) * " + String(center-10) + ";var y = " + String(center) + " + Math.sin(rad) * " + String(center-10) + ";ctx.beginPath();ctx.moveTo(" + String(center) + "," + String(center) + ");ctx.lineTo(x,y);ctx.strokeStyle='#333333';ctx.lineWidth=1;ctx.stroke();}";
    html += "ctx.beginPath();ctx.arc(" + String(center) + "," + String(center) + "," + String(center-5) + ",0,2*Math.PI);ctx.strokeStyle='#3399ff';ctx.lineWidth=2;ctx.stroke();";
    html += "ctx.beginPath();ctx.arc(" + String(center) + "," + String(center) + ",6,0,2*Math.PI);ctx.fillStyle='#00ff00';ctx.fill();ctx.strokeStyle='#00cc00';ctx.lineWidth=2;ctx.stroke();";
    html += "var xs = " + puntosX + ";var ys = " + puntosY + ";for(var i=0;i<xs.length;i++){ctx.beginPath();ctx.arc(xs[i],ys[i],3,0,2*Math.PI);ctx.fillStyle='#ff0000';ctx.fill();ctx.strokeStyle='#cc0000';ctx.lineWidth=1;ctx.stroke();}";
    html += "}";
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