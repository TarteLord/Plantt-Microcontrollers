#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "preprocessors.h"
#include <DHTesp.h>
#include <BH1750.h>
#include <Wire.h>

class Sensor
{
private:
    // DHTesp* dht = new DHTesp; 
    DHTesp dht; 
    BH1750 lightMeter;

    const int airValue = 2900;	// Sensor in mid air
    const int waterValue = 900; // Sensor in water
    const int NUMBER_OF_SAMPLES = 5;
    const int MAXREADINGS = 5;
    const int soilPin = A0;
    const int dhtPin = GPIO_NUM_19; //was 19 before

    int ReadMoisture();
    TempAndHumidity ReadTempAndHumidity();
    float ReadLux();
    
public:
    Sensor();
    ~Sensor();

    typedef struct
    {
	    int moisture;
	    float lux;
        float humidity;
        float temperature;
    } Readings;


    Readings getSensorData();
    
};


#endif // SENSOR_H