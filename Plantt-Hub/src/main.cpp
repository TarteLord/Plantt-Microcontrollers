
#include <BLEDevice.h>
#include <Arduino.h>
#include <inttypes.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "preprocessors.h"

const char *ssid = "May the WIFI be with you";
const char *password = "Abekat123";

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

static BLERemoteCharacteristic *pCharacteristicTemperature;
static BLERemoteCharacteristic *pCharacteristicHumidity;
static BLERemoteCharacteristic *pCharacteristicMoisture;
static BLERemoteCharacteristic *pCharacteristicLux;
static BLERemoteCharacteristic *pCharacteristicSleep;

BLEScan *pBLEScan;
BLEClient *pClient;

static BLEAdvertisedDevice *myDevice;

typedef struct
{
	int moisture;
	float lux;
	float humidity;
	float temperature;
} Readings;

class MyClientCallback : public BLEClientCallbacks
{
	void onConnect(BLEClient *pclient)
	{
		PrintLn("Connected");
	}

	void onDisconnect(BLEClient *pclient)
	{
		connected = false;
		PrintLn("onDisconnect");
	}
};

bool checkCharacteristic(BLERemoteService *pRemoteService /* pClient as arg? */)
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

bool connectToServer()
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

	if (!checkCharacteristic(pRemoteService))
	{
		return false;
	}

	connected = true;

	return true;
}

Readings readBLEData()
{
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

	return readings;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
	/**
	 * Called for each advertising BLE server.
	 */
	void onResult(BLEAdvertisedDevice advertisedDevice)
	{
		PrintL("BLE Advertised Device found: ");
		PrintLn(advertisedDevice.toString().c_str());

		if (advertisedDevice.getServiceUUID().equals(serviceUUID))
		{
			PrintLn("We found it! UUID: ");
			myDevice = new BLEAdvertisedDevice(advertisedDevice);
			doConnect = true;
			// BLEDevice::getScan()->stop();
			pBLEScan->stop();
		}
	}
};

bool postReadingsAPI(Readings readings)
{
	bool result = false;
	char hostHttp[35] = "http://www.plantt.dk/api/v1/hub/";

	char body[80] = "{\"Temperature\":";
	sprintf(body + strlen(body), "%.1f", readings.temperature);
	strcat(body, ",\"Humidity\":");
	sprintf(body + strlen(body), "%.1f", readings.humidity);
	strcat(body, ",\"Lux\":");
	sprintf(body + strlen(body), "%.1f", readings.lux);
	strcat(body, ",\"Moisture\":");
	sprintf(body + strlen(body), "%d", readings.moisture);
	strcat(body, "}");
	PrintLn(body);

	sprintf(hostHttp + strlen(hostHttp), "%d", 7);

	HTTPClient http;
	http.begin(hostHttp);								// Specify destination for HTTP request
	http.addHeader("Content-Type", "application/json"); // Specify content-type header
	int httpResponseCode = http.POST(body);

	if (httpResponseCode > 0)
	{
		String response = http.getString(); // Get the response to the request

		PrintLn("httpResponseCode:");
		PrintL(httpResponseCode); // Print return code
		PrintLn("response:");
		PrintLn(response); // Print request answer

		if (httpResponseCode >= 200 && httpResponseCode <= 204 || httpResponseCode == 307)
		{
			result = true;
		}
		else if (httpResponseCode == 401) 
		{
			//TODO: Handle not logged in.
		}
	}
	else
	{
		PrintL("Error on sending POST: ");
		PrintLn(httpResponseCode); // Print return code
	}

	http.end(); // Free resources

	return result;
}

/// @brief initiate BLE and setup scan and callback.
void startBLE()
{
	BLEDevice::init("");

	pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(99); // less or equal setInterval value
}

void stopBLE()
{
	btStop();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();
}

int readBLEDevice()
{
	// If the flag "doConnect" is true then we have scanned for and found the desired
	// BLE Server with which we wish to connect.  Now we connect to it.  Once we are
	// connected we set the connected flag to be true.
	/*  PrintLn("Check 1");


	 PrintLn("doConnect: ");
	 PrintLn(doConnect); */

	if (doConnect == true)
	{
		/* PrintLn("Check 2"); */
		if (connectToServer())
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
		Readings readings = readBLEData();
		connected = false;

		// delay(1000);
		WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
		WiFi.setHostname("ESP32 Hub");
		WiFi.mode(WIFI_STA);		// Station mode.
		WiFi.begin(ssid, password); // Connect to Wifi AP
		delay(100);

		PrintLn("Check 1");
		int wifiAttemps = 0;

		while (WiFi.status() != WL_CONNECTED && wifiAttemps <= 10)
		{
			delay(200);
			wifiAttemps++;
			PrintL("Not connected to wifi, attempt: ");
			PrintLn(wifiAttemps);
		}

		if (wifiAttemps >= 11)
		{
			//TODO: Save data for next time.
		}
		

		if (WiFi.status() == WL_CONNECTED)
		{
			// Try again if we failed to post data.
			if (!postReadingsAPI(readings))
			{
				delay(1000);
				PrintL("Failed to post data to API");
				postReadingsAPI(readings);
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
	startBLE();

} // End of setup.

// This is the Arduino main loop function.
void loop()
{
	BLEScanResults foundDevices = pBLEScan->start(5, false);
	PrintL("Devices found: ");
	PrintLn(foundDevices.getCount());
	PrintLn("Scan done!");
	pBLEScan->clearResults(); // delete results from BLEScan buffer to release memory
	delay(2000);

	int value = readBLEDevice(); // TODO: redo this function
	// -----------------------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------------------
	/*

		char bodyTest[100] = "{\"Temperature\":26.1,\"Humidity\":27.0,\"Lux\":99.0,\"Moisture\":22}";


			//WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
			WiFi.setHostname("ESP32 Hub");
			WiFi.mode(WIFI_STA);		// Station mode.
			WiFi.begin(ssid, password); // Connect to Wifi AP
			delay(100);

			int wifiAttemps = 0;

			while (WiFi.status() != WL_CONNECTED && wifiAttemps <= 10)
			{
				delay(200);
				wifiAttemps++;
				PrintL("Not connected to wifi, attempt: ");
				PrintLn(wifiAttemps);
			}

			if (WiFi.status() == WL_CONNECTED)
			{

				WiFiClientSecure *client = new WiFiClientSecure;

				//client->setCACert(plantt_root_ca);
				//client->setInsecure();

				// Todo: maybe set the correct size for the char[]
				char Uri[20] = "/api/v1/hub/";
				char host[30] = "www.plantt.dk";
				char hostHTTPS[30] = "https://www.plantt.dk";
				char hostHttp[35] = "http://www.plantt.dk/api/v1/hub/";


				sprintf(Uri + strlen(Uri), "%d", 7);
				sprintf(hostHttp + strlen(hostHttp), "%d", 7);



					HTTPClient http;


					http.begin(hostHttp); // Specify destination for HTTP request
					//http.begin(*client, host, 443, Uri, true);	 // Specify destination for HTTP request
					http.addHeader("Content-Type", "application/json"); // Specify content-type header
					int httpResponseCode = http.POST("{\"Temperature\":26.1,\"Humidity\":27.0,\"Lux\":99.0,\"Moisture\":22}");

					// int httpResponseCode = http.POST("{ \"username\": \"testtest\", \"password\": \"testPost\"}"); // Send the actual POST request
					// int httpResponseCode = http.GET(); // Send the actual POST request

					if (httpResponseCode > 0)
					{
						String response = http.getString(); // Get the response to the request

						PrintLn("httpResponseCode:");
						PrintL(httpResponseCode); // Print return code
						PrintLn("response:");
						PrintLn(response); // Print request answer
					}
					else
					{
						PrintL("Error on sending POST: ");
						PrintLn(httpResponseCode); // Print return code
					}

					http.end(); // Free resources
				} */

	// -----------------------------------------------------------------------------------------------------------
	// -----------------------------------------------------------------------------------------------------------
	// if (value != 0)
	// {
	//   WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
	//   WiFi.setHostname("ESP32 Hub");
	//   WiFi.mode(WIFI_STA); // Station mode.
	//   WiFi.begin(ssid, password); //Connect to Wifi AP
	//   delay(100);

	//   int wifiAttemps = 0;

	//   while (WiFi.status() != WL_CONNECTED && wifiAttemps <= 10) {
	//   delay(200);
	//   wifiAttemps++;
	//   PrintL("Not connected to wifi, attempt: ");
	//   PrintLn(wifiAttemps);
	//   }
	// }

	delay(5000); // Delay a second between loops.
} // End of loop
