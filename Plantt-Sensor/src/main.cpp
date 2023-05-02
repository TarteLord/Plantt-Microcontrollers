#include <BLEDevice.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "moisture.h"
#include "preprocessors.h"


// TODO: make more services for the different services
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTICSSTRING_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTICSONOFF_UUID "8d45ef7f-57b5-48f1-bf95-baf39be3442d"

BLECharacteristic *pCharacteristicMoisture;
BLECharacteristic *pCharacteristicOnOff;

const int airValue = 2900;	// Sensor in mid air
const int waterValue = 900; // Sensor in water
const int soilPin = A0;
const int NUMBER_OF_SAMPLES = 5;
const int MAXREADINGS = 5;

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 1200

typedef struct
{
	int prevMoisture = 0;
	unsigned long epochTime = 0;
} readings;

RTC_DATA_ATTR int prevReadingsCount = 0;
RTC_DATA_ATTR readings PrevValues[MAXREADINGS];

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

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

void setup()
{
	if (PRINT_ENABLED == true)
	{
		Serial.begin(115200);
	}

	//uint16_t check = getMoisturePercentage(soilPin, waterValue, airValue, NUMBER_OF_SAMPLES);

	delay(1000);

	#if PRINT_ENABLED == true
		// Displays the reason for the wake up
		printWakeupReason();
	#endif


	// Enable wakeup from button
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, HIGH);

	// Enable wakeup from timer
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	PrintLn("ESP32 wake-up in " + String(TIME_TO_SLEEP) + " seconds");

	PrintLn("Starting BLE work!");


	BLEDevice::init("Plantt-Sensor");
	BLEServer *pServer = BLEDevice::createServer();
	BLEService *pService = pServer->createService(SERVICE_UUID);
	pCharacteristicMoisture = pService->createCharacteristic(
		CHARACTERISTICSSTRING_UUID,
		BLECharacteristic::PROPERTY_READ);
	BLEDescriptor moistureDescriptor(BLEUUID((uint16_t)0x2902));
	// https://randomnerdtutorials.com/esp32-ble-server-client/ TODO: Remove
	uint16_t moisture = getMoisturePercentage(soilPin, waterValue, airValue, NUMBER_OF_SAMPLES); //TODO: to this before in light sleep prob?
	moistureDescriptor.setValue("MoisturePercentage");
	pCharacteristicMoisture->addDescriptor(&moistureDescriptor);
	pCharacteristicMoisture->setValue(moisture);

	pCharacteristicOnOff = pService->createCharacteristic(
		CHARACTERISTICSONOFF_UUID,
		BLECharacteristic::PROPERTY_WRITE);

	pCharacteristicOnOff->setValue("false");
	pService->start();
	// BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
	pAdvertising->setMinPreferred(0x12);
	BLEDevice::startAdvertising();
	PrintLn("Characteristic defined! Now you can read it");
}

void loop()
{
	if (pCharacteristicOnOff->getValue() == "true")
	{
		PrintLn();
		PrintLn("Goes into Deep Sleep mode by request");
		PrintLn("----------------------");
		delay(100);
		esp_deep_sleep_start();
	}

	if (millis() > 30000)
	{ // 30s = 30000 ms. 1 min = 60000 ms.
		PrintLn();
		PrintLn("Goes into Deep Sleep mode by timeout");
		PrintLn("----------------------");
		delay(100);
		esp_deep_sleep_start();
	}

	// put your main code here, to run repeatedly:
	delay(2000);
}