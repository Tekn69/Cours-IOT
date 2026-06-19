#include <Arduino.h>
#include <WiFi.h>
#include <nvs_flash.h>
#include "sensors/sensor_manager.h"
#include "storage/storage_manager.h"
#include "web/web_manager.h"
#include "network/mqtt_manager.h"

const char *ap_ssid = "ESP32_Technical_Station";
const char *ap_password = "SecurePassword123";

const char *sta_ssid = "TaharTrb";
const char *sta_password = "123456789";

// Tâche Réseau dédiée : Gère la machine d'état MQTT en arrière-plan sur le Core 0
void networkTask(void *parameter)
{
  for (;;)
  {
    // Si la connexion à la box/VM est active, on fait tourner la pile MQTT
    if (WiFi.status() == WL_CONNECTED)
    {
      handleMQTT();
    }
    else
    {
      Serial.println("[WIFI] Reconnecting to infrastructure network...");
      WiFi.begin(sta_ssid, sta_password);
      vTaskDelay(pdMS_TO_TICKS(10000)); // Attend 10s avant de réessayer en cas de perte
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Fréquence de rafraîchissement de la pile réseau
  }
}

void supervisionTask(void *parameter)
{
  for (;;)
  {
    Serial.printf("[SUPERVISION] Free Heap: %u bytes | Uptime: %lu ms\n",
                  ESP.getFreeHeap(), millis());
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n--- SYSTEM INITIALIZATION START ---");

  // 1. Force Clean NVS Initialization to fix the softAP configuration crash
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    Serial.println("[SYSTEM] NVS corrupted. Erasing and reinitializing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  Serial.println("[SYSTEM] NVS Storage operational.");

  // 2. Clean Wi-Fi Driver Sequence & Mode Double (AP + STA)
  WiFi.disconnect(true, true);
  delay(200);
  WiFi.mode(WIFI_AP_STA); // Mode mixte crucial : Permet la page web locale ET le MQTT externe
  delay(200);

  // Configuration du Point d'Accès local (AP)
  Serial.println("[WIFI] Configuring local Access Point...");
  if (WiFi.softAP(ap_ssid, ap_password))
  {
    Serial.print("[WIFI] AP Successfully Created! SSID: ");
    Serial.println(ap_ssid);
    Serial.print("[WIFI] Interface IP Address (for Web UI): ");
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.println("[WIFI] Critical Error: AP Initialization Failed!");
  }

  // Connexion en tant que Station au réseau de la VM (STA)
  Serial.printf("[WIFI] Connecting to infrastructure network: %s...\n", sta_ssid);
  WiFi.begin(sta_ssid, sta_password);

  // Attente non-bloquante initiale (max 10 secondes pour ne pas figer le boot)
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20)
  {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("\n[WIFI] STA Connected! Station IP (on VM network): ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\n[WIFI] Wi-Fi STA connection pending... Will continue in background.");
  }

  // 3. Initialize Storage, Sensors & MQTT Configuration
  initStorage();
  initSensors();
  initMQTT();

  // 4. Initialize Local Web Server
  initWebServer();
  Serial.println("[WEB] Local Web Server active.");

  // 5. Spin up FreeRTOS Tasks
  startSensorTask();
  Serial.println("[OS] Sensor Task deployed to Core 1.");

  // Tâche de gestion Réseau/MQTT déployée sur le Core 0
  xTaskCreatePinnedToCore(
      networkTask,
      "NetworkTask",
      4096,
      NULL,
      1,
      NULL,
      0);
  Serial.println("[OS] Network/MQTT Task deployed to Core 0.");

  xTaskCreatePinnedToCore(
      supervisionTask,
      "SupervisionTask",
      3072,
      NULL,
      1,
      NULL,
      0);
  Serial.println("[OS] Supervision Task deployed to Core 0.");
  Serial.println("--- SYSTEM INITIALIZATION COMPLETE ---");
}

void loop()
{
  // Left empty as mandated by the project specifications
}