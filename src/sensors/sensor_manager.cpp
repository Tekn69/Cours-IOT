#include "sensor_manager.h"
#include <HCSR04.h>
#include "../network/mqtt_manager.h"

#define LED_PIN 2

static byte triggerPin = 21;
static byte echoCount = 1;
static byte echoPins[1] = {12};
static double distances[1];

static SensorData latestData = {0.0, 0, false};
static portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

void initSensors()
{
    pinMode(LED_PIN, OUTPUT);
    HCSR04.begin(triggerPin, echoPins, echoCount);
}

void sensorTaskFunction(void *parameter)
{
    int timeoutCount = 0;
    bool previousState = false;

    for (;;)
    {
        HCSR04.measureDistanceCm(distances);
        double currentDistance = distances[0];
        bool valid = true;

        if (currentDistance == -1.0)
        {
            timeoutCount++;
            valid = false;
            if (timeoutCount > 3)
            {
                digitalWrite(LED_PIN, LOW);
                previousState = false;
            }
        }
        else
        {
            timeoutCount = 0;

            // ACTION : Publication automatique vers le Broker de votre binôme
            publishDistance(currentDistance);

            if (currentDistance > 0 && currentDistance < 6.0)
            {
                digitalWrite(LED_PIN, HIGH);
                previousState = true;
            }
            else
            {
                digitalWrite(LED_PIN, LOW);
                previousState = false;
            }
        }

        portENTER_CRITICAL(&dataMux);
        latestData.distance = currentDistance;
        latestData.timestamp = millis();
        latestData.isValid = valid;
        portEXIT_CRITICAL(&dataMux);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void startSensorTask()
{
    xTaskCreatePinnedToCore(sensorTaskFunction, "SensorTask", 4096, NULL, 1, NULL, 1);
}

SensorData getLatestSensorData()
{
    SensorData temp;
    portENTER_CRITICAL(&dataMux);
    temp = latestData;
    portEXIT_CRITICAL(&dataMux);
    return temp;
}