/*
 * GraphLCD driver library
 *
 * config.h  -  config file classes
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_CONFIG_H_
#define _GLCDDRIVERS_CONFIG_H_

#include <string>
#include <vector>


namespace GLCD
{

const int kWaitUsleep       = 0;
const int kWaitNanosleep    = 1;
const int kWaitNanosleepRR  = 2;
const int kWaitGettimeofday = 3;


struct tOption
{
    std::string name;
    std::string value;
};

class cDriverConfig
{
public:
    std::string name;
    std::string driver;
    int id;
    std::string device;
    int port;
    int width;
    int height;
    bool upsideDown;
    bool invert;
    int brightness;
    int contrast;
    bool backlight;
    int adjustTiming;
    int refreshDisplay;
    std::vector <tOption> options;

public:
    cDriverConfig();
    cDriverConfig(const cDriverConfig & rhs);
    ~cDriverConfig();
    cDriverConfig & operator=(const cDriverConfig & rhs);
    bool Parse(const std::string & line);
    int GetInt(const std::string & value);
    bool GetBool(const std::string & value);
};

class cConfig
{
public:
    int waitMethod;
    int waitPriority;
    std::vector <cDriverConfig> driverConfigs;

public:
    cConfig();
    ~cConfig();
    bool Load(const std::string & filename);
    bool Save(const std::string & filename);
    bool Parse(const std::string & line);
    int GetInt(const std::string & value);
    bool GetBool(const std::string & value);
    int GetConfigIndex(const std::string & name);
};

extern cConfig Config;

} // end of namespace

#endif
