#include "config.h"

Config::Config()
{
    if (!SPIFFS.begin()) 
    {
        PrintLn("Failed to mount SPIFFS");
        return;
    }
}

Config::~Config()
{
}
