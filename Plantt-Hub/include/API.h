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
	char expireTS[30];
	unsigned long _expireTS;
	unsigned long _epochTime;


	bool _loggedIn;
	
	const char* GetAccessToken(const char *pIdentity, const char *pSecret);
	bool ValidateLoginJson(const char *jsonString);
	bool AccessTokenValid();

public:
	API(const char *pIdentity, const char *pSecret);
	~API();

	bool PostReadingsAPI(Readings readings);
	bool SetAccessToken(const char *pIdentity, const char *pSecret);

	
};

#endif // API_H