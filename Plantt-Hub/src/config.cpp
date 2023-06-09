#include "config.h"

/// @brief ctor initiates SPIFFS, and formats on error
Config::Config()
{
	if (!SPIFFS.begin(true))
	{
		PrintLn("Failed to mount SPIFFS");
		return;
	}
	ReadConfig();

	SPIFFS.end();
}

/// @brief Retrieves the singleton instance of the Config class.
/// @return A reference to the singleton instance.
Config &Config::GetInstance()
{
	static Config instance;
	return instance;
}

/// @brief Reads the configuration parameters from the config.txt file.
/// @note The config.txt file should contain the parameters in the following order: SSID, password, identity, secret.
void Config::ReadConfig()
{
	File configFile = SPIFFS.open("/config.txt", FILE_READ);
	if (!configFile)
	{
		Serial.println("Failed to open config file");
		return;
	}

	String ssidTemp = configFile.readStringUntil('\n');
	String passwordTemp = configFile.readStringUntil('\n');
	String identityTemp = configFile.readStringUntil('\n');
	String secretTemp = configFile.readStringUntil('\n');

	ssidTemp.toCharArray(ssid, ssidTemp.length());
	passwordTemp.toCharArray(password, passwordTemp.length());
	identityTemp.toCharArray(identity, identityTemp.length());
	secretTemp.toCharArray(secret, secretTemp.length());


	configFile.close();
}

/// @brief Writes all parameters to config.txt.
/// @param ssidWIFI SSID for the WiFi connection.
/// @param passwordWIFI Password for the WiFi connection.
/// @param identityAPI Identity used for communicating with the API.
/// @param secretAPI Secret used for communicating with the API.
/// @note DO NOT LEAVE ANY PARAM EMPTY, INPUT IS NOT VALIDATED.
/// @return True if successful, false otherwise.
bool Config::WriteConfig(String ssidWIFI, String passwordWIFI, String identityAPI, String secretAPI)
{
	bool result = false;
	if (!SPIFFS.begin(true))
	{
		PrintLn("Failed to mount SPIFFS");
		return false;
	}
	File configFile = SPIFFS.open("/config.txt", FILE_WRITE);
	if (!configFile)
	{
		Serial.println("Failed to open config file for writing");
		return result;
	}

	if (configFile.println(ssidWIFI) && configFile.println(passwordWIFI) && 
		configFile.println(identityAPI) && configFile.println(secretAPI))
	{
		result = true;
	}

	configFile.close();

	SPIFFS.end();

	return result;
}
