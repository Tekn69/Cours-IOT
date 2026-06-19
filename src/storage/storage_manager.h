#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>

void initStorage();
void saveOfflineData(const String &jsonPayload);
bool hasOfflineData();
void flushOfflineDataToMQTT();

#endif