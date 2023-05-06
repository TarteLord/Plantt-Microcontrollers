#ifndef MOISTURE_H
#define MOISTURE_H

#include <Arduino.h>
#include "preprocessors.h"

uint16_t getMoisturePercentage(uint8_t soilPin, int waterValue, int airValue, int numberOfSamples);

uint16_t readMoistureFromSensor(int soilPin, int waterValue, int airValue);


#endif // MOISTURE_H
