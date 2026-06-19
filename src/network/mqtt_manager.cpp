#include "mqtt_manager.h"
#include "../storage/storage_manager.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Configuration selon votre cahier des charges
const char *mqttServer = "10.74.3.183";
const int mqttPort = 1883;
const char *mqttTopic = "campus/groupe2/ESP32-02/data"; // Format campus/<groupe>/<deviceID>/data

WiFiClient espClient;
PubSubClient mqttClient(espClient);
bool wasConnectedLastLoop = false;

void initMQTT()
{
    mqttClient.setServer(mqttServer, mqttPort);
    // Augmente la taille du tampon pour supporter les structures JSON complexes
    mqttClient.setBufferSize(512);
}

bool isMQTTConnected()
{
    return mqttClient.connected();
}

void handleMQTT()
{
    if (!mqttClient.connected())
    {
        wasConnectedLastLoop = false;
        Serial.print("[MQTT] Connecting to broker...");
        String clientId = "ESP32Station-" + WiFi.macAddress();

        if (mqttClient.connect(clientId.c_str()))
        {
            Serial.println(" Connected!");
        }
        else
        {
            Serial.print(" Failed, rc=");
            Serial.println(mqttClient.state());
        }
    }
    else
    {
        // Détection du moment exact de la reconnexion pour purger LittleFS
        if (!wasConnectedLastLoop)
        {
            wasConnectedLastLoop = true;
            if (hasOfflineData())
            {
                flushOfflineDataToMQTT();
            }
        }
    }
    mqttClient.loop();
}

// Utilisé pour envoyer les lignes JSON brutes extraites de LittleFS
void publishRawPayload(const String &payload)
{
    if (mqttClient.connected())
    {
        // "true" en 3e paramètre active la rétention ou la QoS selon la bibliothèque,
        // PubSubClient valide la livraison de base sur l'infrastructure réseau.
        mqttClient.publish(mqttTopic, payload.c_str(), false);
    }
}

// Formate les données en respectant scrupuleusement la structure imposée par le PDF
void publishDistance(double distance)
{
    JsonDocument doc;
    doc["device"] = "ESP32-Distance-Station";
    doc["ts"] = millis(); // Timestamp local

    JsonObject dataNode = doc["data"].to<JsonObject>();
    dataNode["distance"] = distance; // Adaptation de la clé pour votre capteur

    String payload;
    serializeJson(doc, payload);

    if (mqttClient.connected())
    {
        // Envoi direct si en ligne
        mqttClient.publish(mqttTopic, payload.c_str(), false);
        Serial.println("[MQTT] Data sent to server.");
    }
    else
    {
        // Interception par le mode Offline autonome si le serveur est tombé
        Serial.println("[MQTT] Link down. Redirecting payload to local storage.");
        saveOfflineData(payload);
    }
}