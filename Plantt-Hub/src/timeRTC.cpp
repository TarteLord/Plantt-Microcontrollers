#include "timeRTC.h"

TimeRTC *TimeRTC::instance = nullptr;

/// @brief Constructor for the TimeRTC class.
TimeRTC::TimeRTC() : _ntpClient(_ntpUDP, "dk.pool.ntp.org")
{
	PrintLn("Init RTC");

	_ntpClient.begin(); //TODO: Is there a problem here, when we dont know if wifi is connected?

	UpdateRTC();
}

/// @brief GetInstance function to retrieve the singleton instance of TimeRTC.
/// @return Pointer to the singleton instance of TimeRTC.
TimeRTC *TimeRTC::GetInstance()
{
	if (instance == nullptr)
	{
		PrintLn("New instance");
		instance = new TimeRTC();
	}
	return instance;
} 

/// @brief Update the RTC time using NTP.
/// @return True if the RTC update is successful, false otherwise.
bool TimeRTC::UpdateRTC()
{
	PrintLn("Update RTC:");
	bool result = false;
	if (_ntpClient.update())
	{
		lastNTPEpoch = _ntpClient.getEpochTime();
		lastNTPMillis = esp_timer_get_time() / 1000;
		PrintLn("Updated RTC:");
		result = true;
	}

	_ntpClient.end();

	return result;
}

/// @brief Get the current epoch time.
/// @return The current epoch time.
unsigned long TimeRTC::GetEpochTime()
{
	if (!RTCValidate())
	{
		if (!UpdateRTC())
		{
			delay(200);
			UpdateRTC();
		}
	}

	return lastNTPEpoch + ((esp_timer_get_time() / 1000) / 1000);
}

/// @brief Check the validity of the RTC.
/// @return True if the RTC is valid, false otherwise.
bool TimeRTC::RTCValidate()
{
	if (((esp_timer_get_time() / 1000) - lastNTPMillis) > 18000000) // if older than 5 hours
	{
		PrintLn("RTCValidate:");

		return false;
	}

	return true;
}