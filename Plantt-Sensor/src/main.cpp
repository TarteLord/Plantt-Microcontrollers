#include <BLEDevice.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include "driver/adc.h"
#include "preprocessors.h"
#include "sensor.h"

// static BLEUUID sensors[5] = {BLEUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b"), BLEUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b")}
// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

// The Characteristics we are interrested in.
static BLEUUID temperatureUUID("a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6");
static BLEUUID humidityUUID("46cb85fb-eb1e-4a21-b661-0a1d9478d302");
static BLEUUID moistureUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID luxUUID("1af7ac38-7aac-40ee-901f-942bd87f47b1");
static BLEUUID doneReadingUUID("8d45ef7f-57b5-48f1-bf95-baf39be3442d");
static BLEUUID currentDeviceUUID("960d74fe-b9c1-485f-ad3c-224a7e57a37f");

static BLERemoteCharacteristic *pCharacteristicTemperature;
static BLERemoteCharacteristic *pCharacteristicHumidity;
static BLERemoteCharacteristic *pCharacteristicMoisture;
static BLERemoteCharacteristic *pCharacteristicLux;
static BLERemoteCharacteristic *pCharacteristicDoneReading;
static BLERemoteCharacteristic *pCharacteristicCurrentDevice;

BLEScan *pBLEScan;
BLEClient *pClient;

static BLEAdvertisedDevice *myDevice;

const int MAXREADINGS = 5;

const int buttonPin = 32;

static boolean doneReading = true;
bool broadcastStarted = false;
bool serverConnected = false;
bool doConnect = false;

uint32_t sensorID = 1;
Reading reading = {};

#define uS_TO_S_FACTOR 1000000
// #define TIME_TO_SLEEP 1200
#define TIME_TO_SLEEP 8

void setActiveMode()
{
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

class BLECallbacks : public BLEClientCallbacks
{
	void onConnect(BLEClient *pServer)
	{
		PrintLn("A client has connected");
		serverConnected = true;
	}

	void onDisconnect(BLEClient *pServer)
	{
		PrintLn("A client has disconnected");
		
		PrintLn("Millis onDisconnect");
		PrintLn(millis());
		serverConnected = false;
		BLEDevice::stopAdvertising();
		delay(100);
		hibernation();
		if (doneReading == false) //DO we need this?
		{
			PrintLn("Was disruppet unexpectexly");
			doneReading = true;

			// reboot since there apparently is not other way.
			ESP.restart();
		}
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

void setModemSleep()
{
	btStop();
	// adc_power_off(); TODO: Delete
	WiFi.disconnect(true); // Disconnect from the network
	WiFi.mode(WIFI_OFF);
	setCpuFrequencyMhz(80);
	// Use this if 40Mhz is not supported
	// setCpuFrequencyMhz(40); //TODO: Try with this later, when the communication part is done.
}



/// @brief Checks if the Characteristics are present on the BLE service
/// @param pRemoteService The service to check
/// @return state of the check
bool CheckCharacteristic(BLERemoteService *pRemoteService)
{
	// Obtain a reference to the characteristic in the service of the remote BLE server.

	//----------------------------------------------------------------------------------------------
	// temperature
	//----------------------------------------------------------------------------------------------
	pCharacteristicTemperature = pRemoteService->getCharacteristic(temperatureUUID);
	if (pCharacteristicTemperature == nullptr)
	{
		PrintL("Failed to find our characteristic temperature UUID: ");
		PrintLn(temperatureUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for temperature");

	//----------------------------------------------------------------------------------------------
	// Humidity
	//----------------------------------------------------------------------------------------------
	pCharacteristicHumidity = pRemoteService->getCharacteristic(humidityUUID);
	if (pCharacteristicHumidity == nullptr)
	{
		PrintL("Failed to find our characteristic humidity UUID: ");
		PrintLn(moistureUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for humidity");

	//----------------------------------------------------------------------------------------------
	// Moisture
	//----------------------------------------------------------------------------------------------
	pCharacteristicMoisture = pRemoteService->getCharacteristic(moistureUUID);
	if (pCharacteristicMoisture == nullptr)
	{
		PrintL("Failed to find our characteristic Moisture UUID: ");
		PrintLn(moistureUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for Moisture");

	//----------------------------------------------------------------------------------------------
	// Lux
	//----------------------------------------------------------------------------------------------
	pCharacteristicLux = pRemoteService->getCharacteristic(luxUUID);
	if (pCharacteristicLux == nullptr)
	{
		PrintL("Failed to find our characteristic Lux UUID: ");
		PrintLn(luxUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for Lux");

	//----------------------------------------------------------------------------------------------
	// CurrentDevice
	//----------------------------------------------------------------------------------------------
	pCharacteristicCurrentDevice = pRemoteService->getCharacteristic(currentDeviceUUID);
	if (pCharacteristicCurrentDevice == nullptr)
	{
		PrintL("Failed to find our characteristic CurrentDevice UUID: ");
		PrintLn(currentDeviceUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for CurrentDevice");

	/* //----------------------------------------------------------------------------------------------
	// Done reading
	//----------------------------------------------------------------------------------------------
	pCharacteristicDoneReading = pRemoteService->getCharacteristic(doneReadingUUID);
	if (pCharacteristicDoneReading == nullptr)
	{
		PrintL("Failed to find our characteristic Done reading UUID: ");
		PrintLn(doneReadingUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for Done reading"); */

	return true;
}

/// @brief Connect to BLE device and check Service and Characteristic are defined.
/// @return boolean on the state
bool ConnectToServer()
{
	PrintL("Forming a connection to ");
	PrintLn(myDevice->getAddress().toString().c_str());

	pClient = BLEDevice::createClient();
	PrintLn(" - Created client");

	pClient->setClientCallbacks(new BLECallbacks());

	// Connect to the remove BLE Server.
	pClient->connect(myDevice);
	PrintLn(" - Connected to server");

	// Obtain a reference to the service we are after in the remote BLE server.
	BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
	if (pRemoteService == nullptr)
	{
		PrintL("Failed to find our service UUID: ");
		PrintLn(serviceUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our service");

	if (!CheckCharacteristic(pRemoteService))
	{
		return false;
	}

	return true;
}

/// @brief writes all predefined characteristics on Plantt-Hub
void WriteBLEData(Reading sensorData)
{
	doneReading = false; // Lock in case of timeout in connection
	//----------------------------------------------------------------------------------------------
	// Write the value of temperature from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicTemperature->canWrite())
	{
		pCharacteristicTemperature->writeValue(std::to_string(sensorData.temperature));
		PrintL("Writen value for temperature value was: ");
		PrintLn(sensorData.temperature);
	}

	//----------------------------------------------------------------------------------------------
	// Write the value of humidity from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicHumidity->canWrite())
	{
		pCharacteristicHumidity->writeValue(sensorData.humidity);
		PrintL("Writen value for humidity value was: ");
		PrintLn(sensorData.humidity);
	}

	//----------------------------------------------------------------------------------------------
	// Write the value of moisture from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicMoisture->canWrite())
	{
		pCharacteristicMoisture->writeValue(sensorData.moisture);
		PrintL("Writen value for moisture value was: ");
		PrintLn(sensorData.moisture);
	}

	//----------------------------------------------------------------------------------------------
	// Write the value of lux from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicLux->canWrite())
	{
		pCharacteristicLux->writeValue(sensorData.lux); // TODO: consider if we can get a response.
		PrintL("Writen value for lux value was: ");
		PrintLn(sensorData.lux);
	}

	/* //----------------------------------------------------------------------------------------------
	// Set sensor to sleep.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicDoneReading->canWrite())
	{
		PrintLn("Set sensor to sleep.");
		pCharacteristicDoneReading->writeValue(1, false);
	} */

	pClient->disconnect();
	doneReading = true;
}

/// @brief Checks each BLE device found.
class AdvertisedBLECallbacks : public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		// PrintL("BLE Advertised Device found: ");
		// PrintLn(advertisedDevice.toString().c_str());

		if (advertisedDevice.getServiceUUID().equals(serviceUUID))
		{
			PrintLn("Found service with UUID: ");
			PrintLn(advertisedDevice.toString().c_str());
			myDevice = new BLEAdvertisedDevice(advertisedDevice);
			doConnect = true;
			pBLEScan->stop();
		}
	}
};

/// @brief initiate BLE and   scan and callback.
void StartBLE()
{
	BLEDevice::init("");

	pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedBLECallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(99); // less or equal setInterval value
}

bool AbleToWrite()
{
	bool result = false;

	if (pCharacteristicCurrentDevice->canRead())
	{
		uint32_t currentDeviceID = pCharacteristicCurrentDevice->readUInt32();

		if (currentDeviceID == 0)
		{
			if (pCharacteristicCurrentDevice->canWrite())
			{
				pCharacteristicCurrentDevice->writeValue(sensorID);
				result = true;
			}
		}
		else if (currentDeviceID == sensorID)
		{

			result = true;
		}
	}

	return result;
}

void WriteToHub() {
	if (doConnect == true)
	{
		if (ConnectToServer())
		{
			PrintLn("We are now connected to the BLE Server.");
		}
		else
		{
			PrintLn("We have failed to connect to the server; there is nothin more we will do.");
		}
		doConnect = false;
	}

	if (serverConnected)
	{
		int bleAttempts = 0;
		bool ableToWrite = AbleToWrite();
		while (ableToWrite == false && bleAttempts <= 10)
		{
			delay(500);
			ableToWrite = AbleToWrite();
			bleAttempts++;
		}
		
		if (ableToWrite)
		{
			WriteBLEData(reading);
		} 
		
	}
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

	Sensor *sensor = new Sensor();
	reading = sensor->getSensorData();

	delete sensor;

	setActiveMode();

	PrintLn("Millis Startup");
	PrintLn(millis());
	
	StartBLE();

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
		// Blink a LED here or something.
		if (millis() > 5000)
		{
			// Do setup here.
		}
	}

	 BLEScanResults foundDevices = pBLEScan->start(5, false);
	PrintL("Devices found: ");
	PrintLn(foundDevices.getCount());
	PrintLn("Scan done!");
	//pBLEScan->clearResults(); // delete results from BLEScan buffer to release memory
	//delay(2000);			  // TODO: Can we make this value smaler?

	WriteToHub();


	if (millis() > 8000 && serverConnected == false) // Before it was 100000
	{												 // 30s = 30000 ms. 1 min = 60000 ms.
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