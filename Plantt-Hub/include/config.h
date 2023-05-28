#ifndef CONFIG_H
#define CONFIG_H

#include <FS.h>
#include <SPIFFS.h>
#include "preprocessors.h"

/// @brief Config class for managing configuration settings.
class Config
{
private:
	// Private constructor to prevent instantiation
	Config();
	void ReadConfig();
public:
	/// @brief SSID for the WiFi connection.
	char ssid[34];
	
	/// @brief Password for the WiFi connection.
	char password[65];
	
	/// @brief Identity used for communicating with the API.
	char identity[25];
	
	/// @brief Secret used for communicating with the API.
	char secret[70];

	// Static method to access the singleton instance
	static Config &GetInstance();

	bool WriteConfig(String ssidWIFI, String passwordWIFI, String identityAPI, String secretAPI);
	
	// Delete the copy constructor and assignment operator
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;
};

#endif // CONFIG
