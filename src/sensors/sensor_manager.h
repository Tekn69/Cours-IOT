#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

struct SensorData
{
    double distance;
    unsigned long timestamp;
    bool isValid;
};

void initSensors();
void startSensorTask();
SensorData getLatestSensorData();

#endif