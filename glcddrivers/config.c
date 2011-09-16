/*
 * GraphLCD driver library
 *
 * config.c  -  config file classes
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011      Wolfgang Astleitner <mrwastl AT users sourceforge net>
 */

#include <syslog.h>
#include <fstream>
#include <cstring>
#include <cstdlib>

#include "common.h"
#include "config.h"
#include "drivers.h"


namespace GLCD
{

cDriverConfig::cDriverConfig()
:   name(""),
    driver(""),
    id(kDriverUnknown),
    device(""),
    port(0),
    width(0),
    height(0),
    upsideDown(false),
    invert(false),
    brightness(100),
    contrast(5),
    backlight(true),
    adjustTiming(0),
    refreshDisplay(5)
{
}

cDriverConfig::cDriverConfig(const cDriverConfig & rhs)
{
    name = rhs.name;
    driver = rhs.driver;
    id = rhs.id;
    device = rhs.device;
    port = rhs.port;
    width = rhs.width;
    height = rhs.height;
    upsideDown = rhs.upsideDown;
    invert = rhs.invert;
    brightness = rhs.brightness;
    contrast = rhs.contrast;
    backlight = rhs.backlight;
    adjustTiming = rhs.adjustTiming;
    refreshDisplay = rhs.refreshDisplay;
    for (unsigned int i = 0; i < rhs.options.size(); i++)
        options.push_back(rhs.options[i]);
}

cDriverConfig::~cDriverConfig()
{
}

cDriverConfig & cDriverConfig::operator=(const cDriverConfig & rhs)
{
    if (this == &rhs)
        return *this;

    name = rhs.name;
    driver = rhs.driver;
    id = rhs.id;
    device = rhs.device;
    port = rhs.port;
    width = rhs.width;
    height = rhs.height;
    upsideDown = rhs.upsideDown;
    invert = rhs.invert;
    brightness = rhs.brightness;
    contrast = rhs.contrast;
    backlight = rhs.backlight;
    adjustTiming = rhs.adjustTiming;
    refreshDisplay = rhs.refreshDisplay;
    options.clear();
    for (unsigned int i = 0; i < rhs.options.size(); i++)
        options.push_back(rhs.options[i]);

    return *this;
}

bool cDriverConfig::Parse(const std::string & line)
{
    std::string::size_type pos;
    tOption option;

    pos = line.find("=");
    if (pos == std::string::npos)
        return false;
    option.name = trim(line.substr(0, pos));
    option.value = trim(line.substr(pos + 1));
    //printf("D %s = %s\n", option.name.c_str(), option.value.c_str());

    if (option.name == "Driver")
    {
        int driverCount;
        tDriver * drivers = GetAvailableDrivers(driverCount);
        for (int i = 0; i < driverCount; i++)
        {
            if (option.value == drivers[i].name)
            {
                driver = drivers[i].name;
                id = drivers[i].id;
                break;
            }
        }
    }
    else if (option.name == "Device")
    {
        device = option.value;
    }
    else if (option.name == "Port")
    {
        port = GetInt(option.value);
    }
    else if (option.name == "Width")
    {
        width = GetInt(option.value);
    }
    else if (option.name == "Height")
    {
        height = GetInt(option.value);
    }
    else if (option.name == "UpsideDown")
    {
        upsideDown = GetBool(option.value);
    }
    else if (option.name == "Invert")
    {
        invert = GetBool(option.value);
    }
    else if (option.name == "Brightness")
    {
        brightness = GetInt(option.value);
    }
    else if (option.name == "Contrast")
    {
        contrast = GetInt(option.value);
    }
    else if (option.name == "Backlight")
    {
        backlight = GetBool(option.value);
    }
    else if (option.name == "AdjustTiming")
    {
        adjustTiming = GetInt(option.value);
    }
    else if (option.name == "RefreshDisplay")
    {
        refreshDisplay = GetInt(option.value);
    }
    else
    {
        options.push_back(option);
    }
    return true;
}

int cDriverConfig::GetInt(const std::string & value)
{
    return strtol(value.c_str(), NULL, 0);
}

bool cDriverConfig::GetBool(const std::string & value)
{
    return value == "yes";
}


cConfig::cConfig()
:   waitMethod(kWaitGettimeofday),
    waitPriority(0)
{
}

cConfig::~cConfig()
{
}

bool cConfig::Load(const std::string & filename)
{
    std::fstream file;
    char readLine[1000];
    std::string line;
    bool inSections = false;
    int section = 0;

#if (__GNUC__ < 3)
    file.open(filename.c_str(), std::ios::in);
#else
    file.open(filename.c_str(), std::ios_base::in);
#endif
    if (!file.is_open())
        return false;

    while (!file.eof())
    {
        file.getline(readLine, 1000);
        line = trim(readLine);
        if (line.length() == 0)
            continue;
        if (line[0] == '#')
            continue;
        if (line[0] == '[' && line[line.length() - 1] == ']')
        {
            // no ':' in section names
            if (line.substr(1, line.length() - 2).find(':') != std::string::npos) {
                syslog(LOG_ERR, "Config error: section name may not contain a ':', erraneous line: '%s'\n", line.c_str());
                file.close();
                return false;
            }
            if (!inSections)
                inSections = true;
            else
                section++;
            driverConfigs.resize(section + 1);
            driverConfigs[section].name = line.substr(1, line.length() - 2);
            continue;
        }
        if (!inSections)
        {
            Parse(line);
        }
        else
        {
            driverConfigs[section].Parse(line);
        }
    }

    file.close();
    return true;
}

bool cConfig::Parse(const std::string & line)
{
    std::string::size_type pos;
    tOption option;

    pos = line.find("=");
    if (pos == std::string::npos)
        return false;
    option.name = trim(line.substr(0, pos));
    option.value = trim(line.substr(pos + 1));
    //printf("%s = %s\n", option.name.c_str(), option.value.c_str());

    if (option.name == "WaitMethod")
    {
        waitMethod = GetInt(option.value);
    }
    else if (option.name == "WaitPriority")
    {
        waitPriority = GetInt(option.value);
    }
    else
    {
        syslog(LOG_ERR, "Config error: unknown option %s given!\n", option.value.c_str());
        return false;
    }
    return true;
}

int cConfig::GetInt(const std::string & value)
{
    return strtol(value.c_str(), NULL, 0);
}

bool cConfig::GetBool(const std::string & value)
{
    return value == "yes";
}

int cConfig::GetConfigIndex(const std::string & name)
{
    for (int i = 0; i < (int)driverConfigs.size(); i++)
        if (driverConfigs[i].name == name)
            return i;
    syslog(LOG_ERR, "Config error: configuration %s not found!\n", name.c_str());
    return -1;
}

cConfig Config;

} // end of namespace
