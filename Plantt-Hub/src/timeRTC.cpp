#include "timeRTC.h"

TimeRTC* TimeRTC::instance = nullptr; 

TimeRTC::TimeRTC() : _ntpClient(_ntpUDP, "dk.pool.ntp.org")
{
	PrintLn("Init RTC");

	_ntpClient.begin();

	UpdateRTC();
}

TimeRTC* TimeRTC::GetInstance() {
	if (instance == nullptr)
	{
		PrintLn("New instance");
		instance = new TimeRTC();
	} else { //TODO: Delete else, only for debugging
		PrintLn("same instance");
	}
	return instance;
}

TimeRTC::~TimeRTC()
{
	_ntpClient.end();
}

bool TimeRTC::UpdateRTC()
{
	PrintLn("Update RTC:")
	bool result = false;
	if (_ntpClient.update())
	{
		lastNTPEpoch = _ntpClient.getEpochTime();
		lastNTPMillis = esp_timer_get_time() / 1000;
		result = true;
		
		PrintLn("lastNTPEpoch:");
		PrintLn(lastNTPEpoch);
		PrintLn("lastNTPMillis:");
		PrintLn(lastNTPMillis);
	}

	_ntpClient.end();

	return result;
}

unsigned long TimeRTC::GetEpochTime()
{
	if (RTCValidate())
	{
		UpdateRTC();
	}
	
	PrintLn("GetEpochTime:")
	PrintLn(lastNTPEpoch + ((esp_timer_get_time() / 1000) / 1000));
	return lastNTPEpoch + ((esp_timer_get_time() / 1000) / 1000);
}

bool TimeRTC::RTCValidate()
{
	PrintLn("lastNTPMillis:");
	PrintLn(lastNTPMillis);
	PrintLn("millis():");
	PrintLn((esp_timer_get_time() / 1000));
	PrintLn("result:");
	PrintLn((esp_timer_get_time() / 1000) - lastNTPMillis);


	if (((esp_timer_get_time() / 1000) - lastNTPMillis) > 18000000)
	{
		return false;
	}
	
	return true;
}