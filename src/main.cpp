#include <Arduino.h>
#include <WiFi.h>
#include <nvs_flash.h>
#include "sensors/sensor_manager.h" // Gives access to extern variables and initEmergencyButton()
#include "storage/storage_manager.h"
#include "web/web_manager.h"
#include "network/mqtt_manager.h"

const char *ap_ssid = "ESP32_Technical_Station";
const char *ap_password = "SecurePassword123";

const char *sta_ssid = "TaharTrb";
const char *sta_password = "123456789";

// REMOVED: volatile bool isEmergencyActive = false; -> Handled by sensor_manager
// REMOVED: const int EMERGENCY_BUTTON_PIN = 14;     -> Handled by sensor_manager

// Tâche Réseau dédiée : Gère la machine d'état MQTT en arrière-plan sur le Core 0
void networkTask(void *parameter)
{
  for (;;)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      handleMQTT();
    }
    else
    {
      Serial.println("[WIFI] Reconnecting to infrastructure network...");
      WiFi.begin(sta_ssid, sta_password);
      vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
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

  // 1. Force Clean NVS Initialization
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
  WiFi.mode(WIFI_AP_STA);
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

  // FIXED: Added the hardware initialization call here
  initEmergencyButton();
  Serial.println("[SYSTEM] Emergency Hardware Interrupt configured on GPIO 14.");

  initMQTT();

  // 4. Initialize Local Web Server
  initWebServer();
  Serial.println("[WEB] Local Web Server active.");

  // 5. Spin up FreeRTOS Tasks
  startSensorTask();
  Serial.println("[OS] Sensor Task deployed to Core 1.");

  xTaskCreatePinnedToCore(
      networkTask,
      "NetworkTask",
      4096,
      NULL,
      5, // increase network priority
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
  vTaskDelete(NULL); // delete loop
}

void loop()
{
  // Left empty as mandated by the project specifications
}