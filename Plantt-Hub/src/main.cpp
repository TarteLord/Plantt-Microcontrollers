#include <BLEDevice.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include <driver/adc.h>
#include <SPIFFS.h>
#include <FS.h>
#include "preprocessors.h"
#include "sensorData.h"
#include "API.h"
#include "timeRTC.h"
#include "config.h"
#include "tools.h"

// UUID: generated from: https://www.uuidgenerator.net/
//----------------------------------------------------------------------------------------------
// Sensor service
//----------------------------------------------------------------------------------------------
#define SERVICE_SENSOR_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

//----------------------------------------------------------------------------------------------
// Sensor characteristics
//----------------------------------------------------------------------------------------------
#define CHARACTERISTICS_TEMPERATURE_UUID "a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6"
#define CHARACTERISTICS_HUMIDITY_UUID "46cb85fb-eb1e-4a21-b661-0a1d9478d302"
#define CHARACTERISTICS_MOISTURE_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTICS_LUX_UUID "1af7ac38-7aac-40ee-901f-942bd87f47b1"

//----------------------------------------------------------------------------------------------
// Control service
//----------------------------------------------------------------------------------------------
#define SERVICE_CONTROL_UUID "944c4a5a-4c95-47f2-a295-93e1999bb9d2"

//----------------------------------------------------------------------------------------------
// Control characteristics
//----------------------------------------------------------------------------------------------
#define CHARACTERISTICS_DONE_WRITING_UUID "8d45ef7f-57b5-48f1-bf95-baf39be3442d"
#define CHARACTERISTICS_CURRENTDEVICE_UUID "960d74fe-b9c1-485f-ad3c-224a7e57a37f"

//----------------------------------------------------------------------------------------------
// Characteristics
//----------------------------------------------------------------------------------------------
BLECharacteristic *pCharacteristicTemperature;
BLECharacteristic *pCharacteristicHumidity;
BLECharacteristic *pCharacteristicMoisture;
BLECharacteristic *pCharacteristicLux;
BLECharacteristic *pCharacteristicDoneWriting;
BLECharacteristic *pCharacteristicCurrentDevice;

//----------------------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------------------

API *api;

const int maxAmountReadings = 10;
SensorData readings[maxAmountReadings];
int currentAmountReadings = 0;

bool broadcastStarted = false;
bool clientConnected = false;

int64_t millisOnLastDisconnect = 0;

//----------------------------------------------------------------------------------------------
// Code
//----------------------------------------------------------------------------------------------

/// @brief Add reading to our array
/// @param value containing our reading
void AddReading(SensorData value)
{
	if (currentAmountReadings < maxAmountReadings)
	{
		readings[currentAmountReadings] = value;
		currentAmountReadings++;
	}
}

/// @brief remove reading from our array
/// @param value reading to remove
void RemoveReading(SensorData value)
{
	for (int i = 0; i < currentAmountReadings; i++)
	{
		if (readings[i].sensorID == value.sensorID)
		{
			// Shift elements to fill the gap
			for (int j = i; j < currentAmountReadings - 1; j++)
			{
				readings[j] = readings[j + 1];
			}
			currentAmountReadings--;
			break;
		}
	}
}

/// @brief remove reading from our array
/// @param sensorID ID of reading/SensorData to remove
void RemoveReading(int sensorID)
{
	for (int i = 0; i < currentAmountReadings; i++)
	{
		if (readings[i].sensorID == sensorID)
		{
			// Shift elements to fill the gap
			for (int j = i; j < currentAmountReadings - 1; j++)
			{
				readings[j] = readings[j + 1];
			}
			currentAmountReadings--;
			break;
		}
	}
}

/// @brief clear the array of all values.
void ClearReadings()
{
	for (int i = 0; i < currentAmountReadings - 1; i++)
	{
		readings[i].moisture = 0;
		readings[i].lux = 0.0f;
		readings[i].humidity = 0.0f;
		readings[i].temperature = 0.0f;
		currentAmountReadings--;
	}
}


/// @brief Resets multiple BLE characteristics to their default values.
///
/// This function sets the values of the BLE characteristics to their default values.
/// The default values are as follows:
///
///     Temperature: 0.0f
///     Humidity: 0.0f
///     Moisture: 0
///     Lux: 0.0f
///     DoneWriting: 0
///     CurrentDevice: 0
///
/// @note This function assumes that the BLE characteristics have already been initialized and are accessible through the respective pointers.
///
/// @return None
void ResetBLEChacteristics()
{
	std::__cxx11::string zeroFloatStr = "0.0f";
	std::__cxx11::string zeroIntStr = "0";
	int zeroInt = 0;

	pCharacteristicTemperature->setValue(zeroFloatStr);
	pCharacteristicHumidity->setValue(zeroFloatStr);
	pCharacteristicMoisture->setValue(zeroInt);
	pCharacteristicLux->setValue(zeroFloatStr);
	pCharacteristicDoneWriting->setValue(zeroInt);
	pCharacteristicCurrentDevice->setValue(zeroIntStr);
}

/// @brief Validates BLE data
/// @param dataBLE
/// @return bool state of check
bool ValidateBLEdata(SensorData dataBLE)
{
	try
	{
		if (dataBLE.sensorID == 0)
			return false;
		if (dataBLE.humidity == 0.0f)
			return false;
		if (dataBLE.moisture == 0)
			return false;
		if (dataBLE.lux == 0.0f)
			return false;
	}
	catch (const std::exception &e)
	{
		PrintLn("Error on validating BLE data");
		PrintLn(e.what());
		return false;
	}

	return true;
}


/// @brief read Data from BLE Characteristics and add them to readings[]
bool ReadData()
{
	SensorData dataBLE = {};

	dataBLE.temperature = StringToFloat(pCharacteristicTemperature->getValue());
	PrintLn("temperature:");
	PrintLn(dataBLE.temperature);

	dataBLE.humidity = StringToFloat(pCharacteristicHumidity->getValue());
	PrintLn("humidity:");
	PrintLn(dataBLE.humidity);

	dataBLE.moisture = *pCharacteristicMoisture->getData();
	PrintLn("moisture:");
	PrintLn(dataBLE.moisture);

	dataBLE.lux = StringToFloat(pCharacteristicLux->getValue());
	PrintLn("lux:");
	PrintLn(dataBLE.lux);

	dataBLE.sensorID = StringToInt(pCharacteristicCurrentDevice->getValue());
	PrintLn("Device:");
	PrintLn(dataBLE.sensorID);

	bool doneWriting = (bool)*pCharacteristicDoneWriting->getData();
	PrintLn("doneWriting:");
	PrintLn(doneWriting);

	if (doneWriting == true && ValidateBLEdata(dataBLE))
	{
		TimeRTC *timeRTC = TimeRTC::GetInstance();
		dataBLE.epochTS = timeRTC->GetEpochTime();
		AddReading(dataBLE);

		// Reset, so we are ready for the next amount of data.
		ResetBLEChacteristics();
		return true;
	}

	if (((esp_timer_get_time() / 1000) - millisOnLastDisconnect) > 3000 && doneWriting == false)
	{
		if (ValidateBLEdata(dataBLE))
		{
			TimeRTC *timeRTC = TimeRTC::GetInstance();
			dataBLE.epochTS = timeRTC->GetEpochTime();
			AddReading(dataBLE);
		}

		// Reset, so we are ready for the next amount of data.
		ResetBLEChacteristics();
		return true;
	}

	// Update last time a client disconnected.
	millisOnLastDisconnect = esp_timer_get_time() / 1000;

	return false;
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
		ReadData();
	}
};

void StartBLE()
{
	BLEDevice::init("Plantt");
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new BLECallbacks());
	BLEService *pServiceSensor = pServer->createService(SERVICE_SENSOR_UUID);
	BLEService *pServiceControl = pServer->createService(SERVICE_CONTROL_UUID);

	std::__cxx11::string zeroFloatStr = "0.0f";
	std::__cxx11::string zeroIntStr = "0";
	int zeroInt = 0;

	// Temperature
	pCharacteristicTemperature = pServiceSensor->createCharacteristic(
		CHARACTERISTICS_TEMPERATURE_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor temperatureDescriptor(BLEUUID((uint16_t)0x0543));

	temperatureDescriptor.setValue("TemperatureCelcius");
	pCharacteristicTemperature->addDescriptor(&temperatureDescriptor);
	pCharacteristicTemperature->setValue(zeroFloatStr);

	// Humidity
	pCharacteristicHumidity = pServiceSensor->createCharacteristic(
		CHARACTERISTICS_HUMIDITY_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor humidityDescriptor(BLEUUID((uint16_t)0x0544));

	humidityDescriptor.setValue("HumidityPercentage");
	pCharacteristicHumidity->addDescriptor(&humidityDescriptor);
	pCharacteristicHumidity->setValue(zeroFloatStr);

	// Moisture
	pCharacteristicMoisture = pServiceSensor->createCharacteristic(
		CHARACTERISTICS_MOISTURE_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor moistureDescriptor(BLEUUID((uint16_t)0x1079));

	moistureDescriptor.setValue("MoisturePercentage");
	pCharacteristicMoisture->addDescriptor(&moistureDescriptor);
	pCharacteristicMoisture->setValue(zeroInt);

	// Lux
	pCharacteristicLux = pServiceSensor->createCharacteristic(
		CHARACTERISTICS_LUX_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor luxDescriptor(BLEUUID((uint16_t)0x2731));

	luxDescriptor.setValue("LuxPercentage");
	pCharacteristicLux->addDescriptor(&luxDescriptor);
	pCharacteristicLux->setValue(zeroFloatStr);

	// Done Reading
	pCharacteristicDoneWriting = pServiceControl->createCharacteristic(
		CHARACTERISTICS_DONE_WRITING_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor doneReadingDescriptor(BLEUUID((uint16_t)0x2B05));
	pCharacteristicDoneWriting->addDescriptor(&doneReadingDescriptor);

	pCharacteristicDoneWriting->setValue(zeroInt);

	// Current Device
	pCharacteristicCurrentDevice = pServiceControl->createCharacteristic(
		CHARACTERISTICS_CURRENTDEVICE_UUID,
		BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
	BLEDescriptor currentDeviceDescriptor(BLEUUID((uint16_t)0x2902));
	pCharacteristicCurrentDevice->addDescriptor(&currentDeviceDescriptor);

	pCharacteristicCurrentDevice->setValue(zeroIntStr);

	pServiceSensor->start();
	pServiceControl->start();
	BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
	pAdvertising->addServiceUUID(SERVICE_SENSOR_UUID);
	pAdvertising->addServiceUUID(SERVICE_CONTROL_UUID);
	pAdvertising->setScanResponse(true);
	pAdvertising->setMinPreferred(0x08); // For helping with services discovery.

	BLEDevice::startAdvertising();
	broadcastStarted = true;
	PrintLn("Characteristic defined! Now you can read it");
}

void StopBLE()
{
	BLEDevice::deinit();
	delay(100);
}
void StopWIFI()
{
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	delay(100);
}

bool StartWIFI()
{
	Config &config = Config::GetInstance();

	WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
	WiFi.setHostname("Plantt Hub");
	WiFi.mode(WIFI_STA);		// Station mode.
	WiFi.begin(config.ssid, config.password); // Connect to Wifi AP
	delay(100);

	int wifiAttemps = 0;

	while (WiFi.status() != WL_CONNECTED && wifiAttemps <= 20)
	{
		delay(200);
		wifiAttemps++;
		PrintL("Not connected to wifi, attempt: ");
		PrintLn(wifiAttemps);
	}

	if (wifiAttemps >= 21)
	{
		PrintL("Could not connect to wifi.");
		return false;
	}
	return true;
}

void PostDataToAPI()
{
	StopBLE();
	broadcastStarted = false;

	// Connect to WIFI.
	StartWIFI();

	if (WiFi.status() == WL_CONNECTED)
	{
		int httpResponseCode;
		if (currentAmountReadings == 1)
		{
			httpResponseCode = api->PostReadingAPI(readings[0]);
		}
		else
		{
			httpResponseCode = api->PostReadingsAPI(readings, currentAmountReadings);
		}

		if ((httpResponseCode >= 200 && httpResponseCode <= 204) || httpResponseCode == 307)
		{
			PrintLn("PostReadingsAPI succes");
			ClearReadings();
		}
		else
		{
			PrintLn(httpResponseCode);
			PrintLn("Something went wrong, try again later");

			if (httpResponseCode == 403 || httpResponseCode >= 405 && httpResponseCode <= 500)
			{
				PrintLn("Something is wrong API, we can't do anything about it. discarding data");
				ClearReadings();
			}

			delay(2000);
		}
	}

	StopWIFI();
	StartBLE();
}

void setup()
{
	if (PRINT_ENABLED == true)
	{
		Serial.begin(115200);
		delay(1000);
	}

	// Initiate the RTC singleton
	Config &config = Config::GetInstance();

	//Only for debug, purposes
	if (PRINT_ENABLED == true)
	{
		//If config, is empty, lets just write some data.
		if (!(config.ssid[0] == '\0' &&
			config.password[0] == '\0' &&
			config.identity[0] == '\0' &&
			config.secret[0] == '\0'))
		{
			//This should be handled different, when there is a real pairing way implemented.
			if (config.WriteConfig("May the WIFI be with you", "Abekat123", "O9HpT_OYWm2FbvVko7y32yy2", "uIrHT1U540GaaTM4sCefJjgS-kSn0neL3fK8k-QDyLijq1HCtCPAp7xVO7DUP-EW"))
			{
				PrintLn("Success writing to SPIFFS");
			}
		}
	}

	delay(2000);
	PrintLn("Starting Plantt Hub");

	if (!StartWIFI())
	{
		delay(2000);
		StartWIFI();
	}

	if (WiFi.status() == WL_CONNECTED)
	{
		PrintLn("Connected to Wifi");

		// Initiate the RTC singleton
		TimeRTC *timeRTC = TimeRTC::GetInstance();

		api = new API(config.identity, config.secret);
	}

	StopWIFI();
	delay(1000);

	StartBLE();

	PrintLn("Starting BLE work!");
}

void loop()
{
	if (!broadcastStarted)
	{
		PrintLn("Start advertising");
		BLEDevice::startAdvertising();
		broadcastStarted = true;
	}

	if (clientConnected == false && currentAmountReadings >= 1) // TODO: must pass a certain interval?
	{
		PrintLn("We got data to send");
		PostDataToAPI();
	}

	PrintLn("In the main loop");

	delay(1000);
}
