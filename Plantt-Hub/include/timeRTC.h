#ifndef TIMERTC_H
#define TIMERTC_H

#include <RTClib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

namespace TimeRTC
{
	WiFiUDP ntpUDP;
	NTPClient ntpClient(ntpUDP, "dk.pool.ntp.org"); // NTP server for clock.

	RTC_DS3231 rtc; // Real time clock.

	bool StartRTC();
	bool UpdateRTC();

	unsigned long GetEpochTime();
}

#endif // TIME
