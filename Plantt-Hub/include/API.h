#ifndef API_H
#define API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "preprocessors.h"
#include "timeRTC.h"
#include "wifiFix.h"
#include "sensorData.h"

class API
{
private:
	char _accessToken[400];
	unsigned long _expireEpoch;

	const char *_identity;
	const char *_secret;

	String GetAccessToken();
	bool ValidateLoginJson(String jsonString);
	bool AccessTokenValid();
	char *getJsonFormattedSensorData(SensorData reading);
	void freeString(char* strArr);

public:
	API(const char *pIdentity, const char *pSecret);

	int PostReadingAPI(SensorData reading);
	int PostReadingsAPI(SensorData *readings, int readingsAmount);
	bool SetAccessToken();
	int AddSensor();
};

#endif // API_H