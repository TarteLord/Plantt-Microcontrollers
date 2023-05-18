#include "time.h"

namespace time
{

    bool startRTC()
    {
        // Initialize the RTC module
        if (!rtc.begin())
        {
            Serial.println("Failed to initialize RTC");
            delay(1000);
            ESP.restart();
        }

        ntpClient.begin();
        ntpClient.update();

        rtc.adjust(DateTime(ntpClient.getEpochTime()));

        ntpClient.end();
    }
    bool updateRTC()
    {
        ntpClient.update();

        rtc.adjust(DateTime(ntpClient.getEpochTime()));

        ntpClient.end();
    }
}