#ifndef TIMERTC_H
#define TIMERTC_H

#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

class TimeRTC
{
private:
	NTPClient _ntpClient; // NTP server for clock.
	RTC_DS3231 _rtc; // Real time clock.
public:
	TimeRTC(WiFiUDP *ntpUDP);
	~TimeRTC();

	bool UpdateRTC();
	unsigned long GetEpochTime();
};

#endif // TIMERTC
