#include "timeRTC.h"

TimeRTC *TimeRTC::instance = nullptr;

TimeRTC::TimeRTC() : _ntpClient(_ntpUDP, "dk.pool.ntp.org")
{
	PrintLn("Init RTC");

	_ntpClient.begin(); //TODO: Is there a problem here, when we dont know if wifi is connected?

	UpdateRTC();
}

TimeRTC *TimeRTC::GetInstance()
{
	if (instance == nullptr)
	{
		PrintLn("New instance");
		instance = new TimeRTC();
	}
	return instance;
}

TimeRTC::~TimeRTC()
{
	_ntpClient.end();
}

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

bool TimeRTC::RTCValidate()
{
	if (((esp_timer_get_time() / 1000) - lastNTPMillis) > 18000000) // if older than 5 hours
	{
		PrintLn("RTCValidate:");

		return false;
	}

	return true;
}