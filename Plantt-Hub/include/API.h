#ifndef API_H
#define API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "preprocessors.h"
#include "reading.h"
#include "timeRTC.h"
#include "wifiFix.h"

class API
{
private:
	char _accessToken[400];
	unsigned long _expireEpoch;

	// Maybe make identity and secret into pointers, depends on spiffs
	const char *_identity;
	const char *_secret;

	String GetAccessToken();
	bool ValidateLoginJson(String jsonString);
	bool AccessTokenValid();

public:
	API(const char *pIdentity, const char *pSecret);
	~API();

	int PostReadingAPI(Reading readings, int sensorID);
	bool SetAccessToken();
};

#endif // API_H