#include "tools.h"

/// @brief Convert a std::__cxx11::string to float
/// @param value The string value to convert.
/// @return The converted float value.
float StringToFloat(std::__cxx11::string value)
{
	// After using waay to long on this, it's the most reliable way apparently.
	// This is very silly, to convert a string to a String and then to a float...
	// I blame Arduino and whoever wrote the BLE lib.
	String arduinoString;

	for (int i = 0; i < value.length(); i++)
	{
		arduinoString += (char)value[i];
	}
	return arduinoString.toFloat();
}

/// @brief Convert a std::__cxx11::string to Int
/// @param value The string value to convert.
/// @return The converted Int value.
int StringToInt(std::__cxx11::string value)
{
	String arduinoString;

	for (int i = 0; i < value.length(); i++)
	{
		arduinoString += (char)value[i];
	}
	return arduinoString.toInt();
}