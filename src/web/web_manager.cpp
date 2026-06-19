#include "web_manager.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);

void initWebServer()
{
    // Serve Static files from LittleFS
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // REST API Requirement
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        JsonDocument doc;
        doc["device"] = "ESP32-Distance-Station";
        doc["threshold_cm"] = 6.0;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response); });

    server.begin();
}