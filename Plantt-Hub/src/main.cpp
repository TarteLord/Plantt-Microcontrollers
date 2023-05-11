
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
// The characteristic of the remote service we are interested in.
// static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID temperatureUUID("a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6");
static BLEUUID humidityUUID("46cb85fb-eb1e-4a21-b661-0a1d9478d302");
static BLEUUID moistureUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID luxUUID("1af7ac38-7aac-40ee-901f-942bd87f47b1");
static BLEUUID sleepUUID("8d45ef7f-57b5-48f1-bf95-baf39be3442d");

// The characteristic of the remote service we are interested in.
// static BLEUUID    onOffUUID("8d45ef7f-57b5-48f1-bf95-baf39be3442d");

#define CHARACTERISTICS_TEMPERATURE_UUID "a3c0cf09-d1d8-421e-a3f8-e3d7dad17ba6"
#define CHARACTERISTICS_HUMIDITY_UUID "46cb85fb-eb1e-4a21-b661-0a1d9478d302"
#define CHARACTERISTICS_MOISTURE_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTICS_LUX_UUID "1af7ac38-7aac-40ee-901f-942bd87f47b1"
#define CHARACTERISTICS_SLEEP_UUID "8d45ef7f-57b5-48f1-bf95-baf39be3442d"

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
  if (pCharacteristicSleep->canWrite()) {
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
      //BLEDevice::getScan()->stop();
      pBLEScan->stop();
    }
  } 
};  

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
  /* Serial.println("Check 3");
  
  Serial.println("connected: ");
  Serial.println(connected); */
  if (connected)
  {
    Readings readings = readBLEData();
    connected = false;

    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length()); TODO: set sensor to sleep like this
   /*  if (pCharacteristicMoisture->canRead())
    { } */
      if (1 == 2)
      {
        /*  std::uint16_t value = 0;
         value = pCharacteristicMoisture->readUInt16();
         Serial.print("The characteristic value was moisture: ");
         Serial.println(value); */

        Serial.print("asdasdasdasd");

        // Serial.print("Value is not 0");
        //   btStop();
        //   esp_bt_controller_disable();
        //   esp_bt_controller_deinit();
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

        //   if (WiFi.status() == WL_CONNECTED || 1 == 2) {
        //     HTTPClient http;
        //     char Url[55];
        //     char moistureStr[8];
        //     //char epochTimeStr[11];

        //     itoa(value, moistureStr, 10);                //Convert moisture int to string
        //     //itoa(epochTime, epochTimeStr, 10);              //Convert epochTime int to string
        //     //strcpy(Url, "https://2.108.180.113/api/v1/Account/Return"); //Copy the value to Url
        //     strcpy(Url, "https://2.108.180.113/api/v1/Account/Return?request="); //GET
        //     strcat(Url, moistureStr);                       //Appends the moisture to URL
        //     //strcat(Url, "&epochTime=");                     //Appends the Param for epochTime to URL
        //     //strcat(Url, epochTimeStr);                      //Appends the epochTime to URL

        //     http.begin(Url);                                // Specify destination for HTTP request
        //     //http.addHeader("Content-Type", "text/json");   // Specify content-type header
        //     http.addHeader("Content-Type", "text/plain");   // Specify content-type header

        //     //int httpResponseCode = http.POST("{ \"username\": \"testtest\", \"password\": \"testPost\"}"); // Send the actual POST request
        //     int httpResponseCode = http.GET(); // Send the actual POST request

        //     if (httpResponseCode > 0) {
        //       String response = http.getString();           // Get the response to the request

        //       Serial.print(httpResponseCode);               // Print return code
        //       Serial.println(response);                     // Print request answer
        //     }
        //     else {
        //       Serial.print("Error on sending POST: ");
        //       Serial.println(httpResponseCode);             // Print return code
        //     }

        //     http.end(); // Free resources

        //   }

        // if (pRemoteCharacteristicOnOff->canWrite()) { //TODO: For writing.
        //   Serial.println("Value is damm daniel, setting to true");
        //   String newValue = "true";
        //   pRemoteCharacteristicOnOff->writeValue(newValue.c_str(), newValue.length());
        //}
      }
    }
    else if (doScan)
    {                                 // TODO: something else here instead
      //BLEDevice::getScan()->start(0); // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
    }
    /* Serial.println("doScan");
    Serial.println(doScan); */
    return 0;
  }

  void setup()
  {
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    // BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    
    // pBLEScan->setInterval(1349); // tod ved dette interval
    // pBLEScan->setWindow(449);    // samt dette

    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value

    /*  pBLEScan->start(5, false); */
  } // End of setup.

  // This is the Arduino main loop function.
  void loop()
  {

    BLEScanResults foundDevices = pBLEScan->start(5, false);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    //pBLEScan->clearResults(); // delete results from BLEScan buffer to release memory
    //delay(2000);

    /* code */
    int value = readBLEDevice(); // TODO: redo this function

    // const char* input = "{\"name\": \"John\", \"age\": 30}";

    //   char* json = charArrayToJson(input);

    //   std::cout << json << std::endl;

    //   delete[] json;

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

    delay(1000); // Delay a second between loops.
  }              // End of loop
