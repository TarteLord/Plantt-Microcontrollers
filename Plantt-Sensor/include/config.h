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
	int sensorID;
	
	// Static method to access the singleton instance
	static Config &GetInstance();

	bool WriteConfigg(String sensorIDStr);
	
	// Delete the copy constructor and assignment operator
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;
};

#endif // CONFIG
