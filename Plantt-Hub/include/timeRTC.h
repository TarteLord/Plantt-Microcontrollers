#ifndef TIMERTC_H
#define TIMERTC_H

#include <NTPClient.h>
#include <WiFiUdp.h>
#include "preprocessors.h"

class TimeRTC
{
private:
	static TimeRTC *instance;
	unsigned long lastNTPMillis;
	unsigned long lastNTPEpoch;
	WiFiUDP _ntpUDP;
	NTPClient _ntpClient; // NTP server for clock.
	TimeRTC();
public:
	static TimeRTC* GetInstance();
	~TimeRTC();

	bool UpdateRTC();
	bool RTCValidate();
	unsigned long GetEpochTime();

	//Singleton specifics:
	TimeRTC(const TimeRTC&) = delete; //delete copy constructor
    TimeRTC& operator=(const TimeRTC&) = delete; //delete assignment constructor
};

#endif // TIMERTC
