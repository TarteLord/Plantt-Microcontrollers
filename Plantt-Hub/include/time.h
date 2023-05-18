#ifndef TIME_H
#define TIME_H

#include <RTClib.h>
#include <NTPClient.h>

namespace time
{

	NTPClient ntpClient(ntpUDP, "dk.pool.ntp.org"); // NTP server for clock.

	RTC_DS3231 rtc; // Real time clock.

	bool startRTC();
	bool updateRTC();
}

#endif // TIME
