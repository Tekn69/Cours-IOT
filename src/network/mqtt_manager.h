#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>

void initMQTT();
void handleMQTT();
void publishDistance(double distance);
void publishRawPayload(const String &payload); // Pour la retransmission
bool isMQTTConnected();

#endif