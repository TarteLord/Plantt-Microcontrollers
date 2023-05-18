#include <BLEDevice.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include "driver/adc.h"
#include "preprocessors.h"
#include "sensor.h"

// TODO: make more services for the different services
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTICS_TEMPERATURE_UUID 	"a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6"
#define CHARACTERISTICS_HUMIDITY_UUID 		"46cb85fb-eb1e-4a21-b661-0a1d9478d302"
#define CHARACTERISTICS_MOISTURE_UUID 		"beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTICS_LUX_UUID 			"1af7ac38-7aac-40ee-901f-942bd87f47b1"
#define CHARACTERISTICS_SLEEP_UUID 			"8d45ef7f-57b5-48f1-bf95-baf39be3442d"

BLECharacteristic *pCharacteristicTemperature;
BLECharacteristic *pCharacteristicHumidity;
BLECharacteristic *pCharacteristicMoisture;
BLECharacteristic *pCharacteristicLux;
BLECharacteristic *pCharacteristicSleep;

const int MAXREADINGS = 5;

const int buttonPin = 32;

bool broadcastStarted = false;
bool clientConnected = false;

#define uS_TO_S_FACTOR 1000000
// #define TIME_TO_SLEEP 1200
#define TIME_TO_SLEEP 8

class BLECallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    PrintLn("A client has connected");
	clientConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    PrintLn("A client has disconnected");
	clientConnected = false;
  }
};


/// @brief Prints wake up reason
void printWakeupReason()
{
	esp_sleep_wakeup_cause_t wake_up_source;

	wake_up_source = esp_sleep_get_wakeup_cause();

	switch (wake_up_source)
	{

	case ESP_SLEEP_WAKEUP_EXT0:
		PrintLn("Wake-up from external signal with RTC_IO");
		break;

	case ESP_SLEEP_WAKEUP_EXT1:
		PrintLn("Wake-up from external signal with RTC_CNTL");
		break;

	case ESP_SLEEP_WAKEUP_TIMER:
		PrintLn("Wake up caused by a timer");
		break;

	default:
		PrintF("Wake up not caused by Deep Sleep: %d\n", wake_up_source);
		break;
	}
}

void setModemSleep() {
	btStop();
	//adc_power_off(); TODO: Delete
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);
    setCpuFrequencyMhz(80);
    // Use this if 40Mhz is not supported
    // setCpuFrequencyMhz(40); //TODO: Try with this later, when the communication part is done.
}

void setActiveMode() {
	setCpuFrequencyMhz(240);
	adc_power_on();
    delay(100);	
	btStart();
}

void hibernation()
{
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
	esp_deep_sleep_start();
}

void broadcastBLE(Sensor::Readings readings) {
	PrintLn("millis: in modem sleep");
	PrintLn(millis());
	setActiveMode();

	BLEDevice::init("Plantt");
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new BLECallbacks());
	BLEService *pService = pServer->createService(SERVICE_UUID);

	//Temperature
	pCharacteristicTemperature = pService->createCharacteristic(
		CHARACTERISTICS_TEMPERATURE_UUID,
		BLECharacteristic::PROPERTY_READ);
	BLEDescriptor temperatureDescriptor(BLEUUID((uint16_t)0x0543));
	//BLEDescriptor temperatureDescriptor(BLEUUID((uint16_t)0x2902));

	temperatureDescriptor.setValue("TemperatureCelcius");
	pCharacteristicTemperature->addDescriptor(&temperatureDescriptor);
	pCharacteristicTemperature->setValue(readings.temperature);

	//Humidity
	pCharacteristicHumidity = pService->createCharacteristic(
		CHARACTERISTICS_HUMIDITY_UUID,
		BLECharacteristic::PROPERTY_READ);
	BLEDescriptor humidityDescriptor(BLEUUID((uint16_t)0x0544));
	//BLEDescriptor humidityDescriptor(BLEUUID((uint16_t)0x2902));

	humidityDescriptor.setValue("HumidityPercentage");
	pCharacteristicHumidity->addDescriptor(&humidityDescriptor);
	pCharacteristicHumidity->setValue(readings.humidity);


	//Moisture
	pCharacteristicMoisture = pService->createCharacteristic(
		CHARACTERISTICS_MOISTURE_UUID,
		BLECharacteristic::PROPERTY_READ);
	BLEDescriptor moistureDescriptor(BLEUUID((uint16_t)0x1079));
	//BLEDescriptor moistureDescriptor(BLEUUID((uint16_t)0x2902));

	moistureDescriptor.setValue("MoisturePercentage");
	pCharacteristicMoisture->addDescriptor(&moistureDescriptor);
	pCharacteristicMoisture->setValue(readings.moisture);

	//Lux
	pCharacteristicLux = pService->createCharacteristic(
		CHARACTERISTICS_LUX_UUID,
		BLECharacteristic::PROPERTY_READ);
	//BLEDescriptor luxDescriptor(BLEUUID((uint16_t)0x2731));
	BLEDescriptor luxDescriptor(BLEUUID((uint16_t)0x2902));

	luxDescriptor.setValue("LuxPercentage");
	pCharacteristicLux->addDescriptor(&luxDescriptor);
	pCharacteristicLux->setValue(readings.lux);


	//Sleep
	pCharacteristicSleep = pService->createCharacteristic(
		CHARACTERISTICS_SLEEP_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor sleepDescriptor(BLEUUID((uint16_t)0x2902));
	pCharacteristicSleep->addDescriptor(&sleepDescriptor);

	uint8_t value = 0;
	pCharacteristicSleep->setValue((uint8_t*)&value, sizeof(value));
	pService->start();
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x08); //For helping with services discovery.

	BLEDevice::startAdvertising();
	PrintLn("Characteristic defined! Now you can read it");
}

void setup()
{
	if (PRINT_ENABLED == true)
	{
		Serial.begin(115200);
		delay(1000);
	}


	#if PRINT_ENABLED == true
		// Displays the reason for the wake up
		printWakeupReason();
	#endif

	setModemSleep();

	pinMode(buttonPin, INPUT);

	// Enable wakeup from timer
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	PrintLn("ESP32 wake-up in " + String(TIME_TO_SLEEP) + " seconds");

	PrintLn("Starting BLE work!");

	
}

int buttonState = 0;
void loop()
{	
	buttonState = digitalRead(GPIO_NUM_32);

	if (buttonState == true)
	{
		//Blink a LED here or something.
		if (millis() > 5000)
		{
			//Do setup here.
		}
		
	} 
	else if (broadcastStarted == false)
	{
		broadcastStarted = true;
		Sensor sensor;
		Sensor::Readings readings = sensor.getSensorData();
		broadcastBLE(readings);
	}

	//Try to sleep on disconnect instead and timeout.

	if (*pCharacteristicSleep->getData() == 1) 
	{
		PrintLn();
		PrintLn("millis: since start");
		PrintLn(millis());
		PrintLn("Goes into hibernation mode by request");
		PrintLn("----------------------");
		BLEDevice::stopAdvertising();
		delay(100);
		hibernation();
	}

	if (millis() > 8000 && clientConnected == false) //Before it was 100000
	{ // 30s = 30000 ms. 1 min = 60000 ms.
		PrintLn();
		PrintLn("Goes into hibernation mode by timeout");
		PrintLn("----------------------");
		BLEDevice::stopAdvertising();
		delay(100);
		hibernation();
	}

	if (millis() > 12000)
	{ // 30s = 30000 ms. 1 min = 60000 ms.
		PrintLn();
		PrintLn("Goes into hibernation mode by timeout");
		PrintLn("----------------------");
		BLEDevice::stopAdvertising();
		delay(100);
		hibernation();
	}

	delay(150);
}