#include "timeRTC.h"

TimeRTC::TimeRTC(WiFiUDP *ntpUDP) : _ntpClient(*ntpUDP, "dk.pool.ntp.org")
{
	// Initialize the RTC module
	if (!_rtc.begin())
	{
		Serial.println("Failed to initialize RTC");
		delay(1000);
		ESP.restart();
	}

	_ntpClient.begin();
	if (_ntpClient.update())
	{
		_rtc.adjust(DateTime(_ntpClient.getEpochTime()));
	}
	_ntpClient.end();
}

TimeRTC::~TimeRTC()
{
	_ntpClient.end();
}

bool TimeRTC::UpdateRTC()
{
	bool result = false;
	if (_ntpClient.update())
	{
		_rtc.adjust(DateTime(_ntpClient.getEpochTime()));
		result = true;
	}

	_ntpClient.end();

	return result;
}

unsigned long TimeRTC::GetEpochTime()
{
	if (_rtc.lostPower())
	{
		UpdateRTC();
	}

	return _rtc.now().unixtime();
}
