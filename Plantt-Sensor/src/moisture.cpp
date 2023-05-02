#include "moisture.h"

/// @brief Reads the Moisture and convert it to percentage with the calibrated values.
/// @return The median values as persentage
uint16_t getMoisturePercentage(int soilPin, int waterValue, int airValue, int numberOfSamples) {
  uint16_t soilMoisture = 0;
  uint16_t reading = 0;
  for (int i = 1; i < numberOfSamples + 1; i++)
  {
    reading = map(analogRead(soilPin), waterValue, airValue, 100, 0);
    soilMoisture += reading;

    PrintL(reading);
    PrintLn("%");

    delay(1000);
  }

  PrintLn("Total: ");
  PrintL(soilMoisture / numberOfSamples);
  PrintLn("%");

  return soilMoisture / numberOfSamples;
}

//TODO: delete?
uint16_t readMoistureFromSensor(int soilPin, int waterValue, int airValue) {
    return map(analogRead(soilPin), waterValue, airValue, 100, 0);
}