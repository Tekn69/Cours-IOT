#include "storage_manager.h"
#include <LittleFS.h>
#include "../network/mqtt_manager.h"

const char *offlineFile = "/offline_data.json";

void initStorage()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("[STORAGE] Error: LittleFS Mount failed.");
    }
    else
    {
        Serial.println("[STORAGE] LittleFS mounted successfully.");
    }
}

// Vérification silencieuse et propre de l'existence du fichier
bool hasOfflineData()
{
    // FIX: Check existence first to prevent lower-level VFS error logs
    if (!LittleFS.exists(offlineFile))
    {
        return false;
    }

    File file = LittleFS.open(offlineFile, FILE_READ);
    if (!file)
    {
        return false;
    }
    size_t size = file.size();
    file.close();
    return (size > 0);
}

// Sauvegarde robuste
void saveOfflineData(const String &jsonPayload)
{
    // Si le fichier n'existe pas, FILE_WRITE va le créer proprement sans erreur VFS
    if (!LittleFS.exists(offlineFile))
    {
        File cFile = LittleFS.open(offlineFile, FILE_WRITE);
        if (cFile)
        {
            cFile.close();
            Serial.println("[STORAGE] New offline log file created.");
        }
        else
        {
            Serial.println("[STORAGE] Error creating offline file.");
            return;
        }
    }

    File file = LittleFS.open(offlineFile, FILE_APPEND);
    if (file)
    {
        file.println(jsonPayload);
        file.close();
        Serial.println("[STORAGE] Offline data log saved locally.");
    }
    else
    {
        Serial.println("[STORAGE] Error appending offline data!");
    }
}

// Vidage du cache vers le broker MQTT
void flushOfflineDataToMQTT()
{
    // FIX: Pre-check existence here as well
    if (!LittleFS.exists(offlineFile) || !hasOfflineData())
        return;

    File file = LittleFS.open(offlineFile, FILE_READ);
    if (!file)
        return;

    Serial.println("[STORAGE] Network restored. Retransmitting stored data...");

    while (file.available())
    {
        String payload = file.readStringUntil('\n');
        payload.trim();
        if (payload.length() > 0)
        {
            publishRawPayload(payload);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    file.close();

    LittleFS.remove(offlineFile);
    Serial.println("[STORAGE] Offline buffer flushed and cleaned.");
}