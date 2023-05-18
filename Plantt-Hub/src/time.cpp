#include "time.h"

namespace Time
{

    bool StartRTC()
    {
        bool result = false;
        // Initialize the RTC module
        if (!rtc.begin())
        {
            Serial.println("Failed to initialize RTC");
            delay(1000);
            ESP.restart();
        }

        ntpClient.begin();
        if (ntpClient.update())
        {
            rtc.adjust(DateTime(ntpClient.getEpochTime()));
            result = true;
        }
        ntpClient.end();
        
        return result;   


    }
    bool UpdateRTC()
    {
        bool result = false;
        if (ntpClient.update())
        {
            rtc.adjust(DateTime(ntpClient.getEpochTime()));
            result = true;

        }

        ntpClient.end();

        return result;
    }
}