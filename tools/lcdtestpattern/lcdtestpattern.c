/*
 * GraphLCD tool lcdtestpattern
 *
 * lcdtestpattern.c  -  a tool to display some testpattern on an GLCD
 *
 * based on showpic.c of graphlcd-base
 *   (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2007 Alexander Rieger (Alexander.Rieger AT inka.de)
 */

//-----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <dlfcn.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/glcd.h>
#include <glcdgraphics/image.h>
#include <glcddrivers/config.h>
#include <glcddrivers/common.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>

//-----------------------------------------------------------------------------
static const char *prgname = "lcdtestpattern";
static const char *version = "0.1.0";

static const int kDefaultSleepMs = 0;
static const char * kDefaultConfigFile = "/etc/graphlcd.conf";

static volatile bool stopProgramm = false;

//-----------------------------------------------------------------------------
static void sighandler(int signal)
{
	switch (signal)
	{
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
			stopProgramm = true;
	} // switch
} // sighandler()

//-----------------------------------------------------------------------------
void usage()
{
    fprintf(stdout, "\n");
    fprintf(stdout, "%s v%s\n", prgname, version);
    fprintf(stdout, "%s is a tool to show test patterns on a LCD.\n", prgname);
    fprintf(stdout, "\n");
    fprintf(stdout, "  Usage: %s [-c CONFIGFILE] [-d DISPLAY] [-s SLEEP] [-uie]\n\n", prgname);
    fprintf(stdout, "  -c  --config      specifies the location of the config file\n");
    fprintf(stdout, "                    (default: /etc/graphlcd.conf)\n");
    fprintf(stdout, "  -d  --display     specifies the output display (default is the first one)\n");
    fprintf(stdout, "  -u  --upsidedown  rotates the output by 180 degrees (default: no)\n");
    fprintf(stdout, "  -i  --invert      inverts the output (default: no)\n");
    fprintf(stdout, "  -e  --endless     show all images in endless loop (default: no)\n");
    fprintf(stdout, "  -s  --sleep       set sleeptime between two patterns [ms] (default: %d ms)\n", kDefaultSleepMs);
    fprintf(stdout, "  -b  --brightness  set brightness for display if driver support it [%%]\n");
    fprintf(stdout, "                    (default: config file value)\n");
    fprintf(stdout, "\n" );
    fprintf(stdout, "  examples: %s -c /etc/graphlcd.conf\n", prgname);
    fprintf(stdout, "            %s -c /etc/graphlcd.conf -d LCD_T6963 -u -i\n", prgname);
    fprintf(stdout, "\n" );
} // usage()

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    static struct option long_options[] =
    {
        {"config",     required_argument, NULL, 'c'},
        {"display",    required_argument, NULL, 'd'},
        {"sleep",      required_argument, NULL, 's'},
        {"endless",          no_argument, NULL, 'e'},
        {"upsidedown",       no_argument, NULL, 'u'},
        {"invert",           no_argument, NULL, 'i'},
        {"brightness", required_argument, NULL, 'b'},
        {NULL}
    };

    std::string configName = "";
    std::string displayName = "";
    bool upsideDown = false;
    bool invert = false;
    int brightness = -1;
    bool delay = false;
    int sleepMs = kDefaultSleepMs;
    bool endless = false;
    unsigned int displayNumber = 0;

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "c:d:s:euib:", long_options, &option_index)) != -1)
    {
        switch(c)
        {
          case 'c':
              configName = optarg;
              break;

          case 'd':
              displayName = optarg;
              break;

          case 'u':
              upsideDown = true;
              break;

          case 'i':
              invert = true;
              break;

          case 's':
              sleepMs = atoi(optarg);
              delay = true;
              break;

          case 'e':
              endless = true;
              break;

          case 'b':
              brightness = atoi(optarg);
              if (brightness < 0) brightness = 0;
              if (brightness > 100) brightness = 100;
              break;

          default:
              usage();
              return 1;
        } // switch
    } // while

    if (configName.length() == 0)
    {
        configName = kDefaultConfigFile;
        syslog(LOG_INFO, "Error: No config file specified, using default (%s).\n", configName.c_str());
    } // if

    if (GLCD::Config.Load(configName) == false)
    {
        fprintf(stdout, "Error loading config file!\n");
        return 2;
    } // if

    if (GLCD::Config.driverConfigs.size() > 0)
    {
        if (displayName.length() > 0)
        {
            for (displayNumber = 0; displayNumber < GLCD::Config.driverConfigs.size(); displayNumber++)
            {
              if (GLCD::Config.driverConfigs[displayNumber].name == displayName)
                break;
            } // for

            if (displayNumber == GLCD::Config.driverConfigs.size())
            {
              fprintf(stdout, "ERROR: Specified display %s not found in config file!\n", displayName.c_str());
              return 3;
            } // if
        }
        else
        {
            fprintf(stdout, "WARNING: No display specified, using first one.\n");
            displayNumber = 0;
        } // if
    }
    else
    {
        fprintf(stdout, "ERROR: No displays specified in config file!\n");
        return 4;
    } // if

    GLCD::Config.driverConfigs[displayNumber].upsideDown ^= upsideDown;
    GLCD::Config.driverConfigs[displayNumber].invert     ^= invert;

    if (brightness != -1)
    {
        GLCD::Config.driverConfigs[displayNumber].brightness = brightness;
    } // if

    GLCD::cDriver * lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
    if (!lcd)
    {
        fprintf(stderr, "ERROR: Failed creating display object %s\n", displayName.c_str());
        return 6;
    } // if

    if (lcd->Init() != 0)
    {
        fprintf(stderr, "ERROR: Failed initializing display %s\n", displayName.c_str());
        delete lcd;
        return 7;
    } // if

    lcd->SetBrightness(GLCD::Config.driverConfigs[displayNumber].brightness);

    signal(SIGINT, sighandler);
    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGHUP, sighandler);

    //----- show horizontal lines -----
    for (int aByte = 1; aByte <= 255 && !stopProgramm; ++aByte)
    {
        unsigned char aData = GLCD::ReverseBits(aByte);
        printf("clear and test byte: 0x%02X\n", aByte);
        lcd->Clear();

        for (int y = 0; y < lcd->Height() && !stopProgramm; ++y)
        {
            for (int x = 0; x < lcd->Width() && !stopProgramm; x += 8)
            {
               lcd->Set8Pixels(x, y, aData);
            } // for
        } // for

        lcd->Refresh(true);
        usleep(sleepMs * 1000);
    } // for

    //----- show vertial lines -----
    for (int aByte = 1; aByte <= 255 && !stopProgramm; ++aByte)
    {
        printf("clear and test byte: 0x%02X\n", aByte);
        lcd->Clear();

        for (int y = 0; y < lcd->Height() && !stopProgramm; ++y)
        {
            unsigned char aData = (((1 << (y % 8)) & aByte) == 0) ? 0x00 : 0xFF;
            for (int x = 0; x < lcd->Width() && !stopProgramm; x += 8)
            {
               lcd->Set8Pixels(x, y, aData);
            } // for
        } // for

        lcd->Refresh(true);
        usleep(sleepMs * 1000);
    } // for

    //----- cleanup -----
    lcd->Clear();
    lcd->Refresh(true);
    lcd->DeInit();
    delete lcd;

    return 0;
} // main()
