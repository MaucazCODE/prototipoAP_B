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

WebServer server(80);

String leerStringDeEEPROM(int direccion)
{
    String cadena = "";
    char caracter = EEPROM.read(direccion);
    int i = 0;
    while (caracter != '\0' && i < 100)
    {
        cadena += caracter;
        i++;
        caracter = EEPROM.read(direccion + i);
    }
    return cadena;
}

void escribirStringEnEEPROM(int direccion, String cadena)
{
    int longitudCadena = cadena.length();
    for (int i = 0; i < longitudCadena; i++)
    {
        EEPROM.write(direccion + i, cadena[i]);
    }
    EEPROM.write(direccion + longitudCadena, '\0'); // Null-terminated string
    EEPROM.commit();                                // Guardamos los cambios en la memoria EEPROM
}

void handleRoot()
{
    int radioMax = 2000; // mm
    int canvasSize = 400; // px
    int center = canvasSize / 2;
    float escala = (float)center / radioMax;

    // Prepara los arrays JS con los puntos
    String puntosX = "[";
    String puntosY = "[";
    for (int i = 0; i < numPuntos; i++) {
        float anguloRad = historialAngulos[i] * 3.14159265 / 180.0;
        int x = center + int(historialDistancias[i] * cos(anguloRad) * escala);
        int y = center - int(historialDistancias[i] * sin(anguloRad) * escala);
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
    html += "body { font-family: Arial, sans-serif; background: #f0f0f0; color: #222; text-align: center; padding: 40px; }";
    html += ".card { background: #fff; border-radius: 10px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); display: inline-block; padding: 30px 50px; }";
    html += "h1 { color: #0077cc; }";
    html += ".dato { font-size: 2em; margin: 20px 0; }";
    html += "</style></head><body>";
    html += "<div class='card'>";
    html += "<h1>Radar del Robot</h1>";
    html += "<canvas id='radar' width='" + String(canvasSize) + "' height='" + String(canvasSize) + "' style='background:#e0e0e0; border-radius:50%;'></canvas>";
    html += "<div class='dato'>Puntos escaneados: <b>" + String(numPuntos) + "</b></div>";
    html += "<form method='POST' action='/wifi'>";
    html += "<h2>Conectar a Wi-Fi</h2>";
    html += "Red Wi-Fi: <input type='text' name='ssid'><br>";
    html += "Contraseña: <input type='password' name='password'><br>";
    html += "<input type='submit' value='Conectar'>";
    html += "</form>";
    html += "</div>";
    html += "<script>";
    html += "var c = document.getElementById('radar');";
    html += "var ctx = c.getContext('2d');";
    html += "ctx.clearRect(0,0," + String(canvasSize) + "," + String(canvasSize) + ");";
    html += "ctx.beginPath();";
    html += "ctx.arc(" + String(center) + "," + String(center) + "," + String(center-5) + ",0,2*Math.PI);";
    html += "ctx.strokeStyle='#0077cc';ctx.lineWidth=2;ctx.stroke();";
    html += "var xs = " + puntosX + ";";
    html += "var ys = " + puntosY + ";";
    html += "for(var i=0;i<xs.length;i++){";
    html += "ctx.beginPath();";
    html += "ctx.arc(xs[i],ys[i],6,0,2*Math.PI);";
    html += "ctx.fillStyle='#ff3333';ctx.fill();";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}


int posW = 50;
void handleWifi()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    Serial.print("Conectando a la red Wi-Fi ");
    Serial.println(ssid);
    Serial.print("Clave Wi-Fi ");
    Serial.println(password);
    Serial.print("...");
    WiFi.disconnect(); // Desconectar la red Wi-Fi anterior, si se estaba conectado
    WiFi.begin(ssid.c_str(), password.c_str(), 6);

    int cnt = 0;
    while (WiFi.status() != WL_CONNECTED and cnt < 8)
    {
        delay(1000);
        Serial.print(".");
        cnt++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        // guardar en memoria eeprom la ultima red conectada

        Serial.print("Guardando en memoria eeprom...");
/*        if (posW == 0)
            posW = 50;
        else
            posW = 0;*/
        String varsave = leerStringDeEEPROM(300);
        if (varsave == "a") {
            posW = 0;
            escribirStringEnEEPROM(300, "b");
        }
        else{
            posW=50;
            escribirStringEnEEPROM(300, "a");
        }
        escribirStringEnEEPROM(0 + posW, ssid);
        escribirStringEnEEPROM(100 + posW, password);
        // guardar en memoria eeprom la ultima red conectada

        Serial.println("Conexión establecida");
        server.send(200, "text/plain", "Conexión establecida");
    }
    else
    {
        Serial.println("Conexión no establecida");
        server.send(200, "text/plain", "Conexión no establecida");
    }
}

bool lastRed()
{ // verifica si una de las 2 redes guardadas en la memoria eeprom se encuentra disponible
    // para conectarse en ese momento
    for (int psW = 0; psW <= 50; psW += 50)
    {
        String usu = leerStringDeEEPROM(0 + psW);
        String cla = leerStringDeEEPROM(100 + psW);
        Serial.println(usu);
        Serial.println(cla);
        WiFi.disconnect();
        WiFi.begin(usu.c_str(), cla.c_str(), 6);
        int cnt = 0;
        while (WiFi.status() != WL_CONNECTED and cnt < 5)
        {
            delay(1000);
            Serial.print(".");
            cnt++;
        }
        if (WiFi.status() == WL_CONNECTED){
            Serial.println("Conectado a Red Wifi");
            Serial.println(WiFi.localIP());
            break;
        }
    }
    if (WiFi.status() == WL_CONNECTED)
        return true;
    else
        return false;
}

void initAP(const char *apSsid, const char *apPassword)
{ // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
    Serial.begin(115200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    server.on("/", handleRoot);
    server.on("/wifi", handleWifi);

    server.begin();
    Serial.println("Servidor web iniciado");
}

void loopAP()
{

    server.handleClient();
}

void intentoconexion(const char *apname, const char *appassword)
{

    Serial.begin(115200);
    EEPROM.begin(512);
    Serial.println("ingreso a intentoconexion");
    if (!lastRed())
    {                               // redirige a la funcion
        
        Serial.println("Conectarse desde su celular a la red creada");
        Serial.println("en el navegador colocar la ip:");
        Serial.println("192.168.4.1");
        initAP(apname, appassword); // nombre de wifi a generarse y contrasena
    }
    while (WiFi.status() != WL_CONNECTED) // mientras no se encuentre conectado a un red
    {
        loopAP(); // genera una red wifi para que se configure desde la app movil
    }
}