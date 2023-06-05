#ifndef BLEPAIRING_H
#define BLEPAIRING_H

void StopBLEPairing();
void StartBLESensorPairing(int sensorID);


class PairingCallbacks : public BLEServerCallbacks
{
public:
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);
};

#endif
