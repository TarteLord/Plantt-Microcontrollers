#ifndef CONFIG_H
#define CONFIG_H

#include <FS.h>
#include <SPIFFS.h>
#include "preprocessors.h"

class Config
{
private:
	/* data */
public:
	const char *ssid;
	const char *password;
	const char *identity;
	const char *secret;

	void readConfig();
	void writeConfig();

	Config(/* args */);
	~Config();
};

#endif // CONFIG
