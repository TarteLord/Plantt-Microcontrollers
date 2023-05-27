#include <BLEDevice.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include "driver/adc.h"
#include "preprocessors.h"
#include "reading.h"

// TODO: make more services for the different services
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTICS_TEMPERATURE_UUID "a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6"
#define CHARACTERISTICS_HUMIDITY_UUID "46cb85fb-eb1e-4a21-b661-0a1d9478d302"
#define CHARACTERISTICS_MOISTURE_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTICS_LUX_UUID "1af7ac38-7aac-40ee-901f-942bd87f47b1"
#define CHARACTERISTICS_DONE_READING_UUID "8d45ef7f-57b5-48f1-bf95-baf39be3442d"
#define CHARACTERISTICS_CURRENTDEVICE_UUID "960d74fe-b9c1-485f-ad3c-224a7e57a37f"

BLECharacteristic *pCharacteristicTemperature;
BLECharacteristic *pCharacteristicHumidity;
BLECharacteristic *pCharacteristicMoisture;
BLECharacteristic *pCharacteristicLux;
BLECharacteristic *pCharacteristicDoneReading;
BLECharacteristic *pCharacteristicCurrentDevice;

Reading readings[5]; //consider adding SensorID here.

bool broadcastStarted = false;
bool clientConnected = false;

float stringToFloat(std::string value) 
{
	if (value.length() >= 4) {
		return *(float*)(value.data());
	}
	return 0.0;
} 


void readData()
{

	Reading reading = {};
	//float asdasd = stof(pCharacteristicTemperature->getValue());

	float asd = stringToFloat(pCharacteristicTemperature->getValue());

	
	PrintLn("temperature string to float:");
	Serial.print(asd);

      
	
/* 	PrintLn("temperature1:");
	
	std::string temperature = pCharacteristicTemperature->getValue();
	float tempTest = 0.0f;
	try
	{
		PrintLn("temperature2:");
		tempTest = strtof(temperature.c_str(), 0);
	}
	catch(const std::invalid_argument& e)
	{
		PrintLn("invalid_argument:");
		PrintLn(e.what());
		
	}
	catch (const std::out_of_range& e) 
	{
		PrintLn("out_of_range:");
		PrintLn(e.what());
	}	
	
	PrintLn("temperature3:");
	Serial.print(tempTest); */

	//reading.temperature = strtof(temperature, nullptr);
  	//memcpy(&reading.temperature, temperature.data(), sizeof(float));
	//reading.temperature = stof(pCharacteristicTemperature->getValue());
	//memcpy(&reading.temperature, (*pCharacteristicTemperature->getValue()).data(), sizeof(float));;
	//PrintLn("temperature:");
	//PrintLn(reading.temperature);

	// reading.humidity = stof(pCharacteristicHumidity->getValue());
	// //memcpy(&reading.humidity, *pCharacteristicHumidity->getValue().data(), sizeof(float));;
	// PrintLn("humidity:");
	// PrintLn(reading.humidity);

	// reading.moisture = *pCharacteristicMoisture->getData();
	// PrintLn("moisture:");
	// PrintLn(reading.moisture);

	// reading.lux = stof(pCharacteristicLux->getValue());
	// //memcpy(&reading.lux, *pCharacteristicLux->getValue().data(), sizeof(float));;
	// PrintLn("lux:");
	// PrintLn(reading.lux);

	// int sleep = *pCharacteristicDoneReading->getData();
	// PrintLn("sleep:");
	// PrintLn(sleep);

	// int device = *pCharacteristicCurrentDevice->getData();
	// PrintLn("Device:");
	// PrintLn(device);

	//Lav en validering af alle felter ikke er 0.
	//Hvis de er ikke er det, sæt dem alle til 0. og tilføj værdierne i et array. så vi senere kan sende dem til API

	//readings
}

class BLECallbacks : public BLEServerCallbacks
{
	void onConnect(BLEServer *pServer)
	{
		PrintLn("A client has connected");
		clientConnected = true;
	}

	void onDisconnect(BLEServer *pServer)
	{
		PrintLn("A client has disconnected");
		clientConnected = false;
		broadcastStarted = false;
		readData();
	}
};

void broadcastBLE()
{
	BLEDevice::init("Plantt");
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new BLECallbacks());
	BLEService *pService = pServer->createService(SERVICE_UUID);

	float zeroFloat = 0.0f;
	int zeroInt = 0;

	// Temperature
	pCharacteristicTemperature = pService->createCharacteristic(
		CHARACTERISTICS_TEMPERATURE_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor temperatureDescriptor(BLEUUID((uint16_t)0x0543));
	// BLEDescriptor temperatureDescriptor(BLEUUID((uint16_t)0x2902));

	temperatureDescriptor.setValue("TemperatureCelcius");
	pCharacteristicTemperature->addDescriptor(&temperatureDescriptor);
	pCharacteristicTemperature->setValue(zeroFloat);

	// Humidity
	pCharacteristicHumidity = pService->createCharacteristic(
	    CHARACTERISTICS_HUMIDITY_UUID,
	    BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor humidityDescriptor(BLEUUID((uint16_t)0x0544));
	// BLEDescriptor humidityDescriptor(BLEUUID((uint16_t)0x2902));

	humidityDescriptor.setValue("HumidityPercentage");
	pCharacteristicHumidity->addDescriptor(&humidityDescriptor);
	pCharacteristicHumidity->setValue(zeroFloat);

	// Moisture
	pCharacteristicMoisture = pService->createCharacteristic(
	    CHARACTERISTICS_MOISTURE_UUID,
	    BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor moistureDescriptor(BLEUUID((uint16_t)0x1079));
	// BLEDescriptor moistureDescriptor(BLEUUID((uint16_t)0x2902));

	moistureDescriptor.setValue("MoisturePercentage");
	pCharacteristicMoisture->addDescriptor(&moistureDescriptor);
	pCharacteristicMoisture->setValue(zeroInt);

	// Lux
	pCharacteristicLux = pService->createCharacteristic(
	    CHARACTERISTICS_LUX_UUID,
	    BLECharacteristic::PROPERTY_WRITE);
	// BLEDescriptor luxDescriptor(BLEUUID((uint16_t)0x2731));
	BLEDescriptor luxDescriptor(BLEUUID((uint16_t)0x2902));

	luxDescriptor.setValue("LuxPercentage");
	pCharacteristicLux->addDescriptor(&luxDescriptor);
	pCharacteristicLux->setValue(zeroFloat);

	// Current Device
	pCharacteristicCurrentDevice = pService->createCharacteristic(
	    CHARACTERISTICS_CURRENTDEVICE_UUID,
	    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
	BLEDescriptor currentDeviceDescriptor(BLEUUID((uint16_t)0x2902));
	pCharacteristicCurrentDevice->addDescriptor(&currentDeviceDescriptor);

	pCharacteristicCurrentDevice->setValue(zeroInt);

	// Done Reading
	pCharacteristicDoneReading = pService->createCharacteristic(
	    CHARACTERISTICS_DONE_READING_UUID,
	    BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor doneReadingDescriptor(BLEUUID((uint16_t)0x2902));
	pCharacteristicDoneReading->addDescriptor(&doneReadingDescriptor);

	//uint8_t value8 = 0; //TODO: redo this maybe
	//pCharacteristicDoneReading->setValue((uint8_t*)&value8, sizeof(value8));
	pCharacteristicDoneReading->setValue(zeroInt);

	pService->start();
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x08); // For helping with services discovery.

	BLEDevice::startAdvertising();
	broadcastStarted = true;
	PrintLn("Characteristic defined! Now you can read it");
}

void setup()
{
	if (PRINT_ENABLED == true)
	{
		Serial.begin(115200);
		delay(1000);
	}

	broadcastBLE();

	PrintLn("Starting BLE work!");
}

void loop()
{
	if (!broadcastStarted)
	{
		BLEDevice::startAdvertising();
	}
	

	PrintLn("In the main loop");

	delay(1000);
}
