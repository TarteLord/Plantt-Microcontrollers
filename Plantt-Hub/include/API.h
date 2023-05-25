#ifndef API_H
#define API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "preprocessors.h"
#include "readings.h"
#include "timeRTC.h"

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

	bool PostReadingsAPI(Readings readings);
	bool SetAccessToken();
};

#endif // API_H