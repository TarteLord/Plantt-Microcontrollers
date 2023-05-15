
#include "BLEDevice.h"
#include "Arduino.h"
#include "inttypes.h"
#include "HTTPClient.h"
#include "WiFi.h"
#include "cJSON.h"
#include "cJSON_Utils.h"
// #include "BLEScan.h"

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

const char *plantt_root_ca =
	"-----BEGIN CERTIFICATE-----\n"
	"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\n"
	"-----END CERTIFICATE-----\n";

typedef struct
{
	int moisture;
	float lux;
	float humidity;
	float temperature;
} Readings;

static void notifyCallback(
	BLERemoteCharacteristic *pBLERemoteCharacteristic,
	uint8_t *pData,
	size_t length,
	bool isNotify)
{
	Serial.print("Notify callback for characteristic ");
	Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
	Serial.print(" of data length ");
	Serial.println(length);
	Serial.print("data: ");
	Serial.println((char *)pData);
}

class MyClientCallback : public BLEClientCallbacks
{
	void onConnect(BLEClient *pclient)
	{
		Serial.println("Connected");
	}

	void onDisconnect(BLEClient *pclient)
	{
		connected = false;
		Serial.println("onDisconnect");
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
		Serial.print("Failed to find our characteristic temperature UUID: ");
		Serial.print("Failed to find our characteristic temperature UUID: ");
		Serial.println(temperatureUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our characteristic for temperature");

	//----------------------------------------------------------------------------------------------
	// Humidity
	//----------------------------------------------------------------------------------------------
	pCharacteristicHumidity = pRemoteService->getCharacteristic(humidityUUID);
	if (pCharacteristicHumidity == nullptr)
	{
		Serial.print("Failed to find our characteristic humidity UUID: ");
		Serial.println(moistureUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our characteristic for humidity");

	//----------------------------------------------------------------------------------------------
	// Moisture
	//----------------------------------------------------------------------------------------------
	pCharacteristicMoisture = pRemoteService->getCharacteristic(moistureUUID);
	if (pCharacteristicMoisture == nullptr)
	{
		Serial.print("Failed to find our characteristic Moisture UUID: ");
		Serial.println(moistureUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our characteristic for Moisture");

	//----------------------------------------------------------------------------------------------
	// Lux
	//----------------------------------------------------------------------------------------------
	pCharacteristicLux = pRemoteService->getCharacteristic(luxUUID);
	if (pCharacteristicLux == nullptr)
	{
		Serial.print("Failed to find our characteristic Lux UUID: ");
		Serial.println(luxUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our characteristic for Lux");

	//----------------------------------------------------------------------------------------------
	// Sleep
	//----------------------------------------------------------------------------------------------
	pCharacteristicSleep = pRemoteService->getCharacteristic(sleepUUID);
	if (pCharacteristicSleep == nullptr)
	{
		Serial.print("Failed to find our characteristic sleep UUID: ");
		Serial.println(sleepUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our characteristic for sleep");

	return true;
}

bool connectToServer()
{
	Serial.print("Forming a connection to ");
	Serial.println(myDevice->getAddress().toString().c_str());

	pClient = BLEDevice::createClient();
	Serial.println(" - Created client");

	pClient->setClientCallbacks(new MyClientCallback());

	// Connect to the remove BLE Server.
	pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
	Serial.println(" - Connected to server");

	// Obtain a reference to the service we are after in the remote BLE server.
	BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
	if (pRemoteService == nullptr)
	{
		Serial.print("Failed to find our service UUID: ");
		Serial.println(serviceUUID.toString().c_str());
		pClient->disconnect();
		return false;
	}
	Serial.println(" - Found our service");

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
		Serial.print("The characteristic temp float value was: ");
		readings.temperature = pCharacteristicTemperature->readFloat();
		Serial.println(readings.temperature);
	}

	//----------------------------------------------------------------------------------------------
	// Read the value of humidity from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicHumidity->canRead())
	{
		readings.humidity = pCharacteristicHumidity->readFloat();
		Serial.print("The characteristic humidity value was: ");
		Serial.println(readings.humidity);
	}

	//----------------------------------------------------------------------------------------------
	// Read the value of moisture from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicMoisture->canRead())
	{
		readings.moisture = (int)pCharacteristicMoisture->readUInt32();
		Serial.print("The characteristic moisture value was: ");
		Serial.println(readings.moisture);
	}

	//----------------------------------------------------------------------------------------------
	// Read the value of lux from the characteristic.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicLux->canRead())
	{
		readings.lux = (int)pCharacteristicLux->readFloat();
		Serial.print("The characteristic lux value was: ");
		Serial.println(readings.lux);
	}

	//----------------------------------------------------------------------------------------------
	// Set sensor to sleep.
	//----------------------------------------------------------------------------------------------
	if (pCharacteristicSleep->canWrite())
	{
		Serial.println("Set sensor to sleep.");
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
		Serial.print("BLE Advertised Device found: ");
		Serial.println(advertisedDevice.toString().c_str());

		if (advertisedDevice.getServiceUUID().equals(serviceUUID))
		{
			Serial.println("We found it! UUID: ");
			myDevice = new BLEAdvertisedDevice(advertisedDevice);
			doConnect = true;
			// BLEDevice::getScan()->stop();
			pBLEScan->stop();
		}
	}
};

void startBLE() {
	BLEDevice::init("");

	// Retrieve a Scanner and set the callback we want to use to be informed when we
	// have detected a new device.  Specify that we want active scanning and start the
	// scan to run for 5 seconds.
	// BLEScan* pBLEScan = BLEDevice::getScan();

	pBLEScan = BLEDevice::getScan();
	pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
	pBLEScan->setActiveScan(true);
	pBLEScan->setInterval(100);
	pBLEScan->setWindow(99); // less or equal setInterval value
}

void stopBLE() {
	btStop();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();
}

int readBLEDevice()
{
	// If the flag "doConnect" is true then we have scanned for and found the desired
	// BLE Server with which we wish to connect.  Now we connect to it.  Once we are
	// connected we set the connected flag to be true.
	/*  Serial.println("Check 1");


	 Serial.println("doConnect: ");
	 Serial.println(doConnect); */

	if (doConnect == true)
	{
		/* Serial.println("Check 2"); */
		if (connectToServer())
		{
			Serial.println("We are now connected to the BLE Server.");
		}
		else
		{
			Serial.println("We have failed to connect to the server; there is nothin more we will do.");
		}
		doConnect = false;
	}

	if (connected)
	{
		Readings readings = readBLEData();
		connected = false;

		/* btStop();
		esp_bt_controller_disable();
		//esp_bt_controller_deinit();
		BLEDevice::deinit(); */
		//stopBLE();
		delay(1000);
		WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
		WiFi.setHostname("ESP32 Hub");
		WiFi.mode(WIFI_STA);		// Station mode.
		WiFi.begin(ssid, password); // Connect to Wifi AP
		delay(1000);

		Serial.println("Check 1");
		int wifiAttemps = 0;

		while (WiFi.status() != WL_CONNECTED && wifiAttemps <= 10)
		{
			delay(200);
			wifiAttemps++;
			Serial.print("Not connected to wifi, attempt: ");
			Serial.println(wifiAttemps);
		}

		if (WiFi.status() == WL_CONNECTED)
		{
			bool httpClient = true;

		Serial.println("Check 2");
			/* WiFiClientSecure *client = new WiFiClientSecure;

			client->setCACert(plantt_root_ca); */
			//client->setInsecure();

			// Todo: maybe set the correct size for the char[]
			/* char Uri[20] = "/api/v1/hub/";
			char host[30] = "www.plantt.dk";
			char hostHTTPS[30] = "https://www.plantt.dk"; */
			char hostHttp[35] = "http://www.plantt.dk/api/v1/hub/";

			//char Url[40] = "https://2.108.180.113";
			// char body[80] = "{\"body\":{\"Temperature\":";
			char body[80] = "{\"Temperature\":";
			sprintf(body + strlen(body), "%.1f", readings.temperature);
			strcat(body, ",\"Humidity\":");
			sprintf(body + strlen(body), "%.1f", readings.humidity);
			strcat(body, ",\"Lux\":");
			sprintf(body + strlen(body), "%.1f", readings.lux);
			strcat(body, ",\"Moisture\":");
			sprintf(body + strlen(body), "%d", readings.moisture);
			strcat(body, "}");
			Serial.println(body);

			// strcpy(Url, "https://2.108.180.113/api/v1/hub/:id"); // GET
			// sprintf(Url + strlen(Url), "%d", 7);
			//sprintf(Uri + strlen(Uri), "%d", 7);
			sprintf(hostHttp + strlen(hostHttp), "%d", 7);
/* 
			Serial.println("\nStarting connection to server...");
			if (!client.connect(server, 443))
				Serial.println("Connection failed!");
			else
			{
				Serial.println("Connected to server!");
				// Make a HTTP request:
				client.println("GET https://www.howsmyssl.com/a/check HTTP/1.0");
				client.println("Host: www.howsmyssl.com");
				client.println("Connection: close");
				client.println();

				while (client.connected())
				{
					String line = client.readStringUntil('\n');
					if (line == "\r")
					{
						Serial.println("headers received");
						break;
					}
				}
				// if there are incoming bytes available
				// from the server, read them and print them:
				while (client.available())
				{
					char c = client.read();
					Serial.write(c);
				}

				client.stop();
			} */

			if (httpClient)
			{
				HTTPClient http;
				/* code */

				http.begin(hostHttp); // Specify destination for HTTP request
				//http.begin(*client, host, 443, Uri, true);	 // Specify destination for HTTP request
				http.addHeader("Content-Type", "application/json"); // Specify content-type header
				int httpResponseCode = http.POST(body);

				// int httpResponseCode = http.POST("{ \"username\": \"testtest\", \"password\": \"testPost\"}"); // Send the actual POST request
				// int httpResponseCode = http.GET(); // Send the actual POST request

				if (httpResponseCode > 0)
				{
					String response = http.getString(); // Get the response to the request

					Serial.println("httpResponseCode:");
					Serial.print(httpResponseCode); // Print return code
					Serial.println("response:");
					Serial.println(response); // Print request answer
				}
				else
				{
					Serial.print("Error on sending POST: ");
					Serial.println(httpResponseCode); // Print return code
				}

				http.end(); // Free resources
				/* delete Url;
				delete body; */
			}
		}
	}
	else if (doScan)
	{ // TODO: something else here instead
	  // BLEDevice::getScan()->start(0); // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
	}
	/* Serial.println("doScan");
	Serial.println(doScan); */

	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	return 0;
}



void setup()
{
	Serial.begin(115200);
	Serial.println("Starting Arduino BLE Client application...");
	startBLE();

} // End of setup.

// This is the Arduino main loop function.
void loop()
{

	BLEScanResults foundDevices = pBLEScan->start(5, false);
	Serial.print("Devices found: ");
	Serial.println(foundDevices.getCount());
	Serial.println("Scan done!");
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
			Serial.print("Not connected to wifi, attempt: ");
			Serial.println(wifiAttemps);
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

					Serial.println("httpResponseCode:");
					Serial.print(httpResponseCode); // Print return code
					Serial.println("response:");
					Serial.println(response); // Print request answer
				}
				else
				{
					Serial.print("Error on sending POST: ");
					Serial.println(httpResponseCode); // Print return code
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
	//   Serial.print("Not connected to wifi, attempt: ");
	//   Serial.println(wifiAttemps);
	//   }
	// }

	delay(5000); // Delay a second between loops.
} // End of loop
