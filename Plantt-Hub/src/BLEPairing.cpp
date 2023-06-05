#include "BLEPairing.h"

void StopBLEPairing() 
{
	pairingModeActive = false;
	digitalWrite(BUILTIN_LED, LOW);
	StopBLE();
	delay(1000);
	StartBLE();
}

/// @brief Callbacks for BLE server events.
class PairingCallbacks : public BLEServerCallbacks
{
	/// @brief Called when a client is connected to the BLE server.
	/// @param pServer Pointer to the BLE server object.
	void onConnect(BLEServer *pServer)
	{
		PrintLn("A client has connected");
		clientConnected = true;
	}

	/// @brief Called when a client is disconnected from the BLE server.
	/// @param pServer Pointer to the BLE server object.
	void onDisconnect(BLEServer *pServer)
	{
		PrintLn("A client has disconnected");
		clientConnected = false;
		broadcastStarted = false;
		stopBLEPairing();
		
	}
};

void StartBLESensorPairing(int sensorID) {
	BLEDevice::init("Plantt");
	esp_err_t errRc = esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9); // Set max level power BLE
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);						  // Set max level power BLE
	esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);					  // Set max level power BLE
	BLEServer *pServer = BLEDevice::createServer();
	pServer->setCallbacks(new BLECallbacks());
	BLEService *pServiceControl = pServer->createService(SERVICE_CONTROL_UUID);

	std::__cxx11::string zeroIntStr = "0";
    int zeroInt = 0;

	// Sensor ID
	pCharacteristicNextSensorID = pServiceControl->createCharacteristic(
		CHARACTERISTICS_NEXT_SENSORID_UUID,
		BLECharacteristic::PROPERTY_READ);
	BLEDescriptor nextSensorIDDescriptor(BLEUUID((uint16_t)0x2902));

	pCharacteristicTemperature->addDescriptor(&nextSensorIDDescriptor);
	pCharacteristicTemperature->setValue(sensorID);

	// Done Reading
	pCharacteristicDoneWriting = pServiceControl->createCharacteristic(
		CHARACTERISTICS_DONE_WRITING_UUID,
		BLECharacteristic::PROPERTY_WRITE);
	BLEDescriptor doneReadingDescriptor(BLEUUID((uint16_t)0x2B05));
	pCharacteristicDoneWriting->addDescriptor(&doneReadingDescriptor);

	pCharacteristicDoneWriting->setValue(zeroInt);
	
}