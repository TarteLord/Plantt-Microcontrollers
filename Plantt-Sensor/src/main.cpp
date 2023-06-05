#include <BLEDevice.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <WiFi.h>
#include "driver/adc.h"
#include "preprocessors.h"
#include "sensor.h"
#include "tools.h"
#include "config.h"

//----------------------------------------------------------------------------------------------
// The remote services we wish to connect to.
//----------------------------------------------------------------------------------------------
static BLEUUID serviceSensorUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID serviceControlUUID("944c4a5a-4c95-47f2-a295-93e1999bb9d2");

//----------------------------------------------------------------------------------------------
// The sensor Characteristics we are interrested in.
//----------------------------------------------------------------------------------------------
static BLEUUID temperatureUUID("a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6");
static BLEUUID humidityUUID("46cb85fb-eb1e-4a21-b661-0a1d9478d302");
static BLEUUID moistureUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID luxUUID("1af7ac38-7aac-40ee-901f-942bd87f47b1");

//----------------------------------------------------------------------------------------------
// The Control Characteristics we are interrested in.
//----------------------------------------------------------------------------------------------
static BLEUUID doneReadingUUID("8d45ef7f-57b5-48f1-bf95-baf39be3442d");
static BLEUUID currentDeviceUUID("960d74fe-b9c1-485f-ad3c-224a7e57a37f");

//----------------------------------------------------------------------------------------------
// Characteristics
//----------------------------------------------------------------------------------------------
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
int buttonState = 0;
bool buttonActive = false;
unsigned long buttonPressTimer = 0;
unsigned long buttonReleaseTimer = 0;
unsigned long pairingStartedTimer = 0;


bool broadcastStarted = false;
bool serverConnected = false;
bool doConnect = false;
bool setupComplete = false;

int sensorID = 5;
Reading reading = {};

#define uS_TO_S_FACTOR 1000000

#if PRINT_ENABLED == true
#define TIME_TO_SLEEP 60 // 8
#else
// #define TIME_TO_SLEEP 3600
#define TIME_TO_SLEEP 20
#endif

/// @brief Sets the active mode of the device.
/// @note This function sets the CPU frequency to 240MHz, powers on the ADC, introduces a delay of 100 milliseconds,
///       and starts the Bluetooth operation.
void SetActiveMode()
{
	adc_power_on();
	delay(100);
	btStart();
}

/// @brief Sets the modem to sleep mode.
/// @note This function stops Bluetooth operation, disconnects from the network, and turns off WiFi mode.
///       It also sets the CPU frequency to 80MHz.
void setModemSleep()
{
	btStop();
	WiFi.disconnect(true); // Disconnect from the network
	WiFi.mode(WIFI_OFF);
	setCpuFrequencyMhz(80); //We can actually lower the speed without any negative effect.
}

/// @brief Puts the device into hibernation mode.
/// @note This function configures power domains and puts the device into deep sleep mode.
void Hibernation()
{
	PrintLn("goes into hibernation");
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
	esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
	esp_deep_sleep_start();
}

/// @brief Callbacks for BLE client events.
class BLECallbacks : public BLEClientCallbacks
{
	/// @brief Callback function when a client connects.
	/// @param pServer Pointer to the BLEClient object representing the server.
	void onConnect(BLEClient *pServer)
	{
		PrintLn("The client has connected");
		serverConnected = true;
	}

	/// @brief Callback function when a client disconnects.
	/// @param pServer Pointer to the BLEClient object representing the server.
	void onDisconnect(BLEClient *pServer)
	{
		PrintLn("The client has disconnected");

		PrintLn("Millis onDisconnect");
		PrintLn(millis());
		serverConnected = false;
		Hibernation();
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

/// @brief Checks if the sensor characteristics are available in the sensor BLE service.
/// @param pRemoteSensorService A pointer to the sensor BLE service.
/// @note This function obtains references to the temperature, humidity, moisture, and lux characteristics in the sensor BLE service.
/// @return True if all the characteristics are found, False otherwise.
bool CheckSensorCharacteristic(BLERemoteService *pRemoteSensorService)
{
	// Obtain a reference to the characteristic in the service of the remote BLE server.

	//----------------------------------------------------------------------------------------------
	// temperature
	//----------------------------------------------------------------------------------------------
	pCharacteristicTemperature = pRemoteSensorService->getCharacteristic(temperatureUUID);
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
	pCharacteristicHumidity = pRemoteSensorService->getCharacteristic(humidityUUID);
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
	pCharacteristicMoisture = pRemoteSensorService->getCharacteristic(moistureUUID);
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
	pCharacteristicLux = pRemoteSensorService->getCharacteristic(luxUUID);
	if (pCharacteristicLux == nullptr)
	{
		PrintL("Failed to find our characteristic Lux UUID: ");
		PrintLn(luxUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for Lux");

	return true;
}

/// @brief Checks if the control characteristics are available in the control BLE service.
/// @param pRemoteControlService A pointer to the control BLE service.
/// @note This function obtains references to the "Done reading" and "CurrentDevice" characteristics in the control BLE service.
/// @return True if both characteristics are found, False otherwise.
bool CheckControlCharacteristic(BLERemoteService *pRemoteControlService)
{
	// Obtain a reference to the characteristic in the service of the remote BLE server.

	//----------------------------------------------------------------------------------------------
	// Done reading
	//----------------------------------------------------------------------------------------------
	pCharacteristicDoneReading = pRemoteControlService->getCharacteristic(doneReadingUUID);
	if (pCharacteristicDoneReading == nullptr)
	{
		PrintL("Failed to find our characteristic Done reading UUID: ");
		PrintLn(doneReadingUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for Done reading");

	//----------------------------------------------------------------------------------------------
	// CurrentDevice
	//----------------------------------------------------------------------------------------------
	pCharacteristicCurrentDevice = pRemoteControlService->getCharacteristic(currentDeviceUUID);
	if (pCharacteristicCurrentDevice == nullptr)
	{
		PrintL("Failed to find our characteristic CurrentDevice UUID: ");
		PrintLn(currentDeviceUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for CurrentDevice");

	return true;
}

/// @brief Forms a connection to a remote BLE server and verifies the availability of sensor and control characteristics.
/// @note This function connects to the remote BLE server, obtains references to the sensor and control services, and checks for the availability of corresponding characteristics.
/// @return True if the connection is successful and all necessary characteristics are found, False otherwise.
bool ConnectToServer()
{
	PrintL("Forming a connection to ");
	PrintLn(myDevice->getAddress().toString().c_str());

	pClient = BLEDevice::createClient();
	PrintLn(" - Created client");

	pClient->setClientCallbacks(new BLECallbacks());

	// Connect to the remote BLE Server.
	bool _connected = pClient->connect(myDevice);
	PrintLn(" - Connected to server");
	if (!_connected) //sometimes peer device is disconnected before real connection is established. we need to delete BLEClient to close connection in bt stack
	{
		delete pClient;
		delete myDevice;
		return false;
	}

	// Obtain a reference to the sensor service we are after in the remote BLE server.
	BLERemoteService *pRemoteSensorService = pClient->getService(serviceSensorUUID);
	if (pRemoteSensorService == nullptr)
	{
		PrintL("Failed to find our service UUID: ");
		PrintLn(serviceSensorUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our sensor service");

	if (!CheckSensorCharacteristic(pRemoteSensorService))
	{
		pClient->disconnect();
		return false;
	}

	// Obtain a reference to the control service we are after in the remote BLE server.
	BLERemoteService *pRemoteControlService = pClient->getService(serviceControlUUID);
	if (pRemoteControlService == nullptr)
	{
		PrintL("Failed to find our service UUID: ");
		PrintLn(serviceControlUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our control service");

	if (!CheckControlCharacteristic(pRemoteControlService))
	{
		pClient->disconnect();
		return false;
	}

	return true;
}

/// @brief Writes sensor data to the corresponding characteristics of the remote BLE server.
/// @param sensorData The sensor data to be written.
/// @note This function writes the temperature, humidity, moisture, and lux values to their respective characteristics in the remote BLE server. It also sets the "DoneReading" characteristic to indicate that the sensor readings transfer is complete.
void WriteBLEData(Reading sensorData)
{
	//----------------------------------------------------------------------------------------------
	// Write the value of temperature from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicTemperature->canWrite())
	{
		pCharacteristicTemperature->writeValue(std::__cxx11::to_string(sensorData.temperature));
		PrintL("Writen value for temperature value was: ");
		PrintLn(sensorData.temperature);
	}

	//----------------------------------------------------------------------------------------------
	// Write the value of humidity from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicHumidity->canWrite())
	{
		pCharacteristicHumidity->writeValue(std::__cxx11::to_string(sensorData.humidity));
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
		pCharacteristicLux->writeValue(std::__cxx11::to_string(sensorData.lux));
		PrintL("Writen value for lux value was: ");
		PrintLn(sensorData.lux);
	}

	//----------------------------------------------------------------------------------------------
	// Set sensor to DoneReading.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicDoneReading->canWrite())
	{
		PrintLn("Set hub to done reading.");
		pCharacteristicDoneReading->writeValue(1, false);
	}

}

/// @brief validates sensor data to the corresponding characteristics of the remote BLE server.
/// @param sensorData The sensor data to be validated against.
/// @note This function validate the temperature, humidity, moisture, and lux values to their respective characteristics in the remote BLE server.
bool ValidateBLEData(Reading sensorData)
{
	//----------------------------------------------------------------------------------------------
	// validate the value of temperature from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicTemperature->canRead())
	{
		std::__cxx11::string value = pCharacteristicTemperature->readValue();

		if (std::__cxx11::to_string(sensorData.temperature) != value)
		{
			PrintL("validated value for temperature value was: ");
			PrintLn(value.c_str());
			return false;
		}
	}

	//----------------------------------------------------------------------------------------------
	// validate the value of humidity from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicHumidity->canRead())
	{
		std::__cxx11::string value = pCharacteristicHumidity->readValue();

		if (std::__cxx11::to_string(sensorData.humidity) != value)
		{
			PrintL("validated value for humidity value was: ");
			PrintLn(value.c_str());
			return false;
		}
	}

	//----------------------------------------------------------------------------------------------
	// validate the value of moisture from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicMoisture->canWrite())
	{
		pCharacteristicMoisture->writeValue(sensorData.moisture);
		PrintL("Writen value for moisture value was: ");
		PrintLn(sensorData.moisture);
	}

	if (pCharacteristicMoisture->canRead())
	{
		int value = (int)pCharacteristicMoisture->readUInt32();

		if (value != sensorData.moisture)
		{
			PrintL("validated value for moisture value was: ");
			PrintLn(value);
			return false;
		}
	}

	//----------------------------------------------------------------------------------------------
	// validate the value of lux from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicLux->canRead())
	{
		std::__cxx11::string value = pCharacteristicLux->readValue();

		if (std::__cxx11::to_string(sensorData.lux) != value)
		{
			PrintL("validated value for lux value was: ");
			PrintLn(value.c_str());
			return false;
		}
	}

	return true;

}

/// @brief Callbacks for advertised BLE devices.
class AdvertisedBLECallbacks : public BLEAdvertisedDeviceCallbacks
{
	/// @brief Called when an advertised BLE device is found.
	/// @param advertisedDevice The advertised BLE device.
	/// @note Checks if the device's service UUID matches the desired sensor service UUID.
	///       If a match is found, sets the myDevice variable, sets the doConnect flag to true,
	///       and stops the BLE scanning process.
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		if (advertisedDevice.getServiceUUID().equals(serviceSensorUUID))
		{
			PrintLn("Found sensor service with UUID: ");
			PrintLn(advertisedDevice.toString().c_str());
			myDevice = new BLEAdvertisedDevice(advertisedDevice);
			doConnect = true;
			pBLEScan->stop();
		}
	}
};

/// @brief Initializes the BLE device and setup scanning for advertised devices.
/// @note This function initializes the BLE device and configures the scanning parameters
///       before starting the scan for advertised BLE devices.
void SetupBLE()
{
	BLEDevice::init("");
	esp_err_t errRc = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9); // Set max level power BLE
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);						  // Set max level power BLE
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);					  // Set max level power BLE

	pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedBLECallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(99); // less or equal setInterval value
}

/// @brief Checks if the current device is able to write data.
/// @note This function verifies if the current device can read the current device ID.
///       If the current device ID is 0, it checks if the characteristic can be written
///       with the sensor ID. If the current device ID matches the sensor ID, it indicates
///       that the device is able to write data.
/// @return Returns true if the current device is able to write data, false otherwise.
bool AbleToWrite()
{
	bool result = false;

	if (pCharacteristicCurrentDevice->canRead())
	{
		int currentDeviceID = StringToInt(pCharacteristicCurrentDevice->readValue());

		if (currentDeviceID == 0)
		{
			if (pCharacteristicCurrentDevice->canWrite())
			{
				pCharacteristicCurrentDevice->writeValue(std::to_string(sensorID));
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

/// @brief Writes sensor data to the hub if a valid connection is established.
void WriteToHub()
{
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
			//The DHT sensor is just awfull, and sometimes return 0.0F. 
			//So lets just validate our reading before sending it.
			if (reading.humidity == 0.0F || reading.lux == 0.0F || reading.moisture == 0 || reading.temperature == 0.0F)
			{
				Sensor *sensor = new Sensor();
				reading = sensor->getSensorData();

				delete sensor;
			}
			
			WriteBLEData(reading);
			if (!ValidateBLEData(reading))
			{
				WriteBLEData(reading);
			}
			
			pClient->disconnect();
		}
	}
}

/// @brief Initializes the system, sets up the sensor, activates the mode, starts BLE, enables timer wakeup, and prepares for operation.
void setup()
{
#if PRINT_ENABLED == true
	Serial.begin(115200);
	delay(1000);
	// Displays the reason for the wake up
	printWakeupReason();
#endif

	setModemSleep();

	// Initiate the Config singleton
	Config &config = Config::GetInstance();

	if (config.sensorID != 0)
	{
		setupComplete = true;

		//Get sensordata, if we have a sensorID
		Sensor *sensor = new Sensor();
		reading = sensor->getSensorData();

		delete sensor; 

	}
	else 
	{
		setupComplete = false;
		sensorID = 0;
	}
	
 
	PrintLn("Millis after sensor");
	PrintLn(millis());



	SetActiveMode();

	PrintLn("Millis after Active mode");
	PrintLn(millis());

	SetupBLE();

	pinMode(buttonPin, INPUT);

	// Enable wakeup from timer
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	PrintLn("ESP32 wake-up in " + String(TIME_TO_SLEEP) + " seconds");

	PrintLn("Starting BLE work!");
}

/// @brief Executes the main loop of the program, including button state check, BLE scanning, data writing to the hub, and hibernation.
///        The function checks the state of a button, performs BLE scanning to discover devices, prints the count of found devices,
///        writes data to the hub if the server is connected and able to write, and triggers hibernation based on timeout conditions.
void loop()
{
	int buttonState = digitalRead(buttonPin);
	digitalWrite(BUILTIN_LED, LOW);

	if (buttonState == HIGH && buttonActive == false)
	{
		buttonActive = true;
		buttonPressTimer = (esp_timer_get_time() / 1000);
	}

	if (buttonState == LOW && buttonActive == true)
	{
		buttonReleaseTimer = (esp_timer_get_time() / 1000);
		unsigned long pressDuration = buttonReleaseTimer - buttonPressTimer;

		if (pressDuration >= 10000) //10 seconds
		{
			longPressActive = true;
		}
		buttonActive = false;
	}

	if (longPressActive == true)
	{
		digitalWrite(BUILTIN_LED, HIGH);
		broadcastStarted = false;
		pairingModeActive = true;
		pairingStartedTimer = (esp_timer_get_time() / 1000);
		StopBLE();
		StartWIFI();
		int sensorID;

		if (WiFi.status() == WL_CONNECTED)
		{
			sensorID = api->AddSensor();
		}
		PrintLn("sensorID:");
		PrintLn(sensorID);
		if (sensorID == 0)
		{
			PrintLn("Pairing API CALL failed.");
			pairingModeActive = false;
			digitalWrite(BUILTIN_LED, LOW);
			delay(500);
			digitalWrite(BUILTIN_LED, HIGH);
			delay(500);
			digitalWrite(BUILTIN_LED, LOW);
			delay(500);
			digitalWrite(BUILTIN_LED, HIGH);
		}
		

		StopWIFI();
		StartBLESensorPairing();

		longPressActive = false;
	}

	if (setupComplete)
	{
		BLEScanResults foundDevices = pBLEScan->start(5, false);
		PrintL("Devices found: ");
		PrintLn(foundDevices.getCount());
		PrintLn("Scan done!");
		pBLEScan->clearResults(); // delete results from BLEScan buffer to release memory

		WriteToHub();

		if (millis() > 10000 && serverConnected == false) // 10s = 10000 ms. 1 min = 60000 ms.
		{
			PrintLn();
			PrintLn("Goes into hibernation mode by timeout");
			PrintLn("----------------------");
			delay(100);
			Hibernation();
		}

		if (millis() > 12000) // 12s = 12000 ms. 1 min = 60000 ms.
		{
			PrintLn();
			PrintLn("Goes into hibernation mode by timeout");
			PrintLn("----------------------");
			delay(100);
			Hibernation();
		}
	}
	else
	{
		//Call setup here.
	}

	delay(150);
}