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
	
	const char* getAccessToken(const char *pIdentity, const char *pSecret);
	bool validateLoginJson(const char *jsonString);
	bool checkAccessToken();

public:
	API(const char *pIdentity, const char *pSecret);
	~API();

	bool postReadingsAPI(Readings readings);
	bool setAccessToken(const char *pIdentity, const char *pSecret);


	struct login
	{
		char accessToken[400];
		char expireTS[30];
	};


	
};

#endif // API_H