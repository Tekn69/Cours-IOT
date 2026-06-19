#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

struct SensorData
{
    double distance;
    unsigned long timestamp;
    bool isValid;
};

// Global flag to notify other tasks of the emergency state
extern volatile bool isEmergencyActive;

// Function declaration to initialize the hardware interrupt button
void initEmergencyButton();

void initSensors();
void startSensorTask();
SensorData getLatestSensorData();

#endif