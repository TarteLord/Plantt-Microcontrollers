#include "Sensor.h"

/// @brief Initialise Sensor class and setup I2C etc.
Sensor::Sensor()
{
    Wire.begin(GPIO_NUM_16, GPIO_NUM_17);
    

    if (lightMeter->begin(BH1750::ONE_TIME_HIGH_RES_MODE))
    {
        PrintLn(F("BH1750 Advanced begin"));
    }
    else
    {
        PrintLn(F("Error initialising BH1750"));
    }
    delay(250);
    dht->setup(dhtPin, DHTesp::DHT11);
}

/// @brief Deconstructor
Sensor::~Sensor()
{
    lightMeter->~BH1750();
    dht->~DHTesp();

    delete lightMeter;
    delete dht;
}

/// @brief Read soil moisture from sensor, mapped with "calibrated values".
/// @return soil moisture in percent.
int Sensor::ReadMoisture()
{

    int result = 0;

    result = map(analogRead(soilPin), waterValue, airValue, 100, 0);
    PrintLn("Soil Moisture: ");
    PrintL(result);
    PrintLn("");

    return result;
}

/// @brief read lux from BH1750 sensor
/// @return lux value as float.
float Sensor::ReadLux()
{
    float lux = 0.0F;
    if (lightMeter->measurementReady(true))
    {
        lux = lightMeter->readLightLevel();
        PrintLn("");
        PrintL("Light: ");
        PrintL(lux);
        PrintLn(" lx");
    }
    return lux;
}

/// @brief read temperature and humidity from DHT11
/// @return struct with humidty and temperature
TempAndHumidity Sensor::ReadTempAndHumidity()
{
    
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    TempAndHumidity values = dht->getTempAndHumidity();

    // Check if any reads failed and exit early (to try again).
    if (dht->getStatus() != 0)
    {
        PrintLn("DHT11 error status: ");
        PrintL(dht->getStatusString());
        values.humidity = 0.0F;
        values.temperature = 0.0F;
        return values;
    }

    PrintLn("");
    PrintLn("Temperature: ");
    PrintLn(values.temperature);
    PrintLn("Humidity: ");
    PrintL(values.humidity);
    PrintLn("");

    return values;
}

/// @brief reads Temperature, Humidity, Lux, and soil moisture from sensors.
/// @return A struct containing Temperature, Humidity, Lux, and soil moisture
Reading Sensor::getSensorData()
{
    Reading result;

    float luxReading = ReadLux();
    if (luxReading == 0.0f)
    {
        //delay(250);
        luxReading = ReadLux();
    }

    float moistureReading = ReadMoisture();
    if (moistureReading == 0)
    {
        //delay(250);
        moistureReading = ReadMoisture();
    }

    TempAndHumidity tempHumReading = ReadTempAndHumidity();
    if (tempHumReading.humidity == 0.0F || tempHumReading.temperature == 0.0F )
    {
        
        tempHumReading = ReadTempAndHumidity();
    }

    result.humidity = tempHumReading.humidity;
    result.temperature = tempHumReading.temperature;
    result.lux = luxReading;
    result.moisture = moistureReading;

    return result;
}
