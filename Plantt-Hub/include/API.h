#ifndef API_H
#define API_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "preprocessors.h"
#include "readings.h"

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
	bool CheckAccessToken();

public:
	API(const char *pIdentity, const char *pSecret);
	~API();

	bool PostReadingsAPI(Readings readings);
	bool SetAccessToken(const char *pIdentity, const char *pSecret);

/* 
	struct login
	{
		char accessToken[400];
		char expireTS[30];
	}; */


	
};

#endif // API_H