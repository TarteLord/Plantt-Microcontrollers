
#include <BLEDevice.h>
#include <Arduino.h>
#include <inttypes.h>
//#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "preprocessors.h"
#include "readings.h"
#include "API.h"
#include "time.h"

const char *ssid = "May the WIFI be with you";
const char *password = "Abekat123";

const char *identity = "O9HpT_OYWm2FbvVko7y32yy2";
const char *secret = "uIrHT1U540GaaTM4sCefJjgS-kSn0neL3fK8k-QDyLijq1HCtCPAp7xVO7DUP-EW";

WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, "dk.pool.ntp.org"); //NTP server for clock.

RTC_DS3231 rtc; // Real time clock.

char accessToken[400] = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJhYTZlNDYwZi05NjczLTRiNjktOWI2ZS0wZGVmY2Q4N2RhYTYiLCJzdWIiOiJPOUhwVF9PWVdtMkZidlZrbzd5MzJ5eTIiLCJpc3MiOiJpc3N1ZXIuY29tIiwicm9sZSI6IlRva2VuIiwibmJmIjoxNjg0MjM2MjIwLCJleHAiOjE2ODQyMzk4MjAsImlhdCI6MTY4NDIzNjIyMH0.6Pl-UXWhQm163COVYwA8CpyKAWAa4-EVd6NH_BDXpY8";
char expireTS [30] = "2023-05-16T12:26:03.5627978Z";
// static BLEUUID sensors[5] = {BLEUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b"), BLEUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b")};

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");

// The Characteristics we are interrested in.
static BLEUUID temperatureUUID("a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6");
static BLEUUID humidityUUID("46cb85fb-eb1e-4a21-b661-0a1d9478d302");
static BLEUUID moistureUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID luxUUID("1af7ac38-7aac-40ee-901f-942bd87f47b1");
static BLEUUID sleepUUID("8d45ef7f-57b5-48f1-bf95-baf39be3442d");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static boolean doneReading = true;

static BLERemoteCharacteristic *pCharacteristicTemperature;
static BLERemoteCharacteristic *pCharacteristicHumidity;
static BLERemoteCharacteristic *pCharacteristicMoisture;
static BLERemoteCharacteristic *pCharacteristicLux;
static BLERemoteCharacteristic *pCharacteristicSleep;

BLEScan *pBLEScan;
BLEClient *pClient;

static BLEAdvertisedDevice *myDevice;



/// @brief Tracks state of the BLE Client.
class MyClientCallback : public BLEClientCallbacks
{
	void onConnect(BLEClient *pclient)
	{
		connected = true;
		PrintLn("Connected");
	}

	void onDisconnect(BLEClient *pclient)
	{
		connected = false;
		PrintLn("onDisconnect");
		if (doneReading == false)
		{
			PrintLn("Was disruppet unexpectexly");
			doneReading = true;

			//reboot since there apparently is not other way.
			ESP.restart();
		}
		
	}
};

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
	// Sleep
	//----------------------------------------------------------------------------------------------
	pCharacteristicSleep = pRemoteService->getCharacteristic(sleepUUID);
	if (pCharacteristicSleep == nullptr)
	{
		PrintL("Failed to find our characteristic sleep UUID: ");
		PrintLn(sleepUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	PrintLn(" - Found our characteristic for sleep");

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

	pClient->setClientCallbacks(new MyClientCallback());

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

/// @brief Reads all predefined characteristics on Plantt-Sensor
/// @return A Readings struct containing data
Readings ReadBLEData()
{
	doneReading = false; //Lock in case of timeout in connection
	Readings readings = {};
	//----------------------------------------------------------------------------------------------
	// Read the value of temperature from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicTemperature->canRead())
	{
		PrintL("The characteristic temp float value was: ");
		readings.temperature = pCharacteristicTemperature->readFloat();
		PrintLn(readings.temperature);
	}

	//----------------------------------------------------------------------------------------------
	// Read the value of humidity from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicHumidity->canRead())
	{
		readings.humidity = pCharacteristicHumidity->readFloat();
		PrintL("The characteristic humidity value was: ");
		PrintLn(readings.humidity);
	}

	//----------------------------------------------------------------------------------------------
	// Read the value of moisture from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicMoisture->canRead())
	{
		readings.moisture = (int)pCharacteristicMoisture->readUInt32();
		PrintL("The characteristic moisture value was: ");
		PrintLn(readings.moisture);
	}

	//----------------------------------------------------------------------------------------------
	// Read the value of lux from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicLux->canRead())
	{
		readings.lux = (int)pCharacteristicLux->readFloat();
		PrintL("The characteristic lux value was: ");
		PrintLn(readings.lux);
	}

	//----------------------------------------------------------------------------------------------
	// Set sensor to sleep.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicSleep->canWrite())
	{
		PrintLn("Set sensor to sleep.");
		pCharacteristicSleep->writeValue(1, false);
	}

	pClient->disconnect();
	doneReading = true;

	return readings;
}

/// @brief Checks each BLE device found.
class AdvertisedBLECallbacks : public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		//PrintL("BLE Advertised Device found: ");
		//PrintLn(advertisedDevice.toString().c_str());

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

void StopBLE()
{
	btStop();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();
}

int ReadBLEDevice()
{
	// If the flag "doConnect" is true then we have scanned for and found the desired
	// BLE Server with which we wish to connect.  Now we connect to it.  Once we are
	// connected we set the connected flag to be true.

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

	if (connected)
	{
		Readings readings = ReadBLEData();

		PrintLn("readings.humidity");
		PrintLn(readings.humidity);
		PrintLn("readings.temperature");
		PrintLn(readings.temperature);
		PrintLn("readings.moisture");
		PrintLn(readings.moisture);
		PrintLn("readings.lux");
		PrintLn(readings.lux);

		//esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
		

		//Connect to WIFI.
		WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
		WiFi.setHostname("ESP32 Hub");
		WiFi.mode(WIFI_STA);		// Station mode.
		WiFi.begin(ssid, password); // Connect to Wifi AP
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
			//TODO: Save data for next time, since we can't connect to wifi.
		}
		

		if (WiFi.status() == WL_CONNECTED)
		{
			
			//API::getAccessToken(identity, secret); todo: delete
			API api(identity, secret);
			
			if (!api.PostReadingsAPI(readings))
			{
				// Try again if we failed to post data.
				delay(1000);
				PrintL("Failed to post data to API");
				api.PostReadingsAPI(readings);
			}
		}
	}

	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	return 0;
}

void setup()
{
	if (PRINT_ENABLED == true)
	{
		Serial.begin(115200);
		delay(1000);
	}

	
	PrintLn("Starting Plantt Hub");
	
	if (Time::StartRTC())
	{
		PrintLn("Time succesfully started");
	}
	
	
	StartBLE();

} // End of setup.

void loop()
{
	BLEScanResults foundDevices = pBLEScan->start(5, false);
	PrintL("Devices found: ");
	PrintLn(foundDevices.getCount());
	PrintLn("Scan done!");
	pBLEScan->clearResults(); // delete results from BLEScan buffer to release memory
	delay(2000); //TODO: Can we make this value smaler?

	int value = ReadBLEDevice(); // TODO: redo this function

	
	delay(5000); // Delay a second between loops.
} // End of loop
