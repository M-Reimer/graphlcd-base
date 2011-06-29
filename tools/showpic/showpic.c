/*
 * GraphLCD tool showpic
 *
 * showpic.c  -  a tool to show image files in GLCD format on a LCD
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *  
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *               Andreas 'randy' Weinberger
 */

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
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>
#include <glcdgraphics/extformats.h>

static const char *prgname = "showpic";
static const char *version = "0.1.2";

static const int kDefaultSleepMs = 100;
static const char * kDefaultConfigFile = "/etc/graphlcd.conf";

static volatile bool stopProgramm = false;

static void sighandler(int signal)
{
	switch (signal)
	{
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
			stopProgramm = true;
	}
}

void usage()
{
	fprintf(stdout, "\n");
	fprintf(stdout, "%s v%s\n", prgname, version);
	fprintf(stdout, "%s is a tool to show an image on an LCD.\n", prgname);
	fprintf(stdout, "The image must be in a special format (*.glcd).\n");
	fprintf(stdout, "You can create such images with the convpic tool.\n\n");
	fprintf(stdout, "  Usage: %s [-c CONFIGFILE] [-d DISPLAY] [-s SLEEP] [-uie] file [more files]\n\n", prgname);
	fprintf(stdout, "  -c  --config      specifies the location of the config file\n");
	fprintf(stdout, "                    (default: /etc/graphlcd.conf)\n");
	fprintf(stdout, "  -d  --display     specifies the output display (default is the first one)\n");
	fprintf(stdout, "  -u  --upsidedown  rotates the output by 180 degrees (default: no)\n");
	fprintf(stdout, "  -i  --invert      inverts the output (default: no)\n");
	fprintf(stdout, "  -e  --endless     show all images in endless loop (default: no)\n");
	fprintf(stdout, "  -s  --sleep       set sleeptime between two images [ms] (default: %d ms)\n", kDefaultSleepMs);
	fprintf(stdout, "  -b  --brightness  set brightness for display (if supported by the driver) [%%]\n");
	fprintf(stdout, "                    (default: config file value)\n");
	fprintf(stdout, "\n" );
	fprintf(stdout, "  examples: %s -c /etc/graphlcd.conf vdr-logo.glcd\n", prgname);
	fprintf(stdout, "            %s -c /etc/graphlcd.conf -d LCD_T6963 -u -i vdr-logo.glcd\n", prgname);
	fprintf(stdout, "\n" );
}

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
	int sleepMs = 100;
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
		}
	}

	if (configName.length() == 0)
	{
		configName = kDefaultConfigFile;
		syslog(LOG_INFO, "Error: No config file specified, using default (%s).\n", configName.c_str());
	}

	if (GLCD::Config.Load(configName) == false)
	{
		fprintf(stdout, "Error loading config file!\n");
		return 2;
	}
	if (GLCD::Config.driverConfigs.size() > 0)
	{
		if (displayName.length() > 0)
		{
			for (displayNumber = 0; displayNumber < GLCD::Config.driverConfigs.size(); displayNumber++)
			{
				if (GLCD::Config.driverConfigs[displayNumber].name == displayName)
					break;
			}
			if (displayNumber == GLCD::Config.driverConfigs.size())
			{
				fprintf(stdout, "ERROR: Specified display %s not found in config file!\n", displayName.c_str());
				return 3;
			}
		}
		else
		{
			fprintf(stdout, "WARNING: No display specified, using first one.\n");
			displayNumber = 0;
		}
	}
	else
	{
		fprintf(stdout, "ERROR: No displays specified in config file!\n");
		return 4;
	}

	if (optind == argc)
	{
		usage();
		fprintf(stderr, "ERROR: You have to specify the image\n");
		return 5;
	}

	GLCD::Config.driverConfigs[displayNumber].upsideDown ^= upsideDown;
	GLCD::Config.driverConfigs[displayNumber].invert ^= invert;
	if (brightness != -1)
		GLCD::Config.driverConfigs[displayNumber].brightness = brightness;
	GLCD::cDriver * lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
	if (!lcd)
	{
		fprintf(stderr, "ERROR: Failed creating display object %s\n", displayName.c_str());
		return 6;
	}
	if (lcd->Init() != 0)
	{
		fprintf(stderr, "ERROR: Failed initializing display %s\n", displayName.c_str());
		delete lcd;
		return 7;
	}
	lcd->SetBrightness(GLCD::Config.driverConfigs[displayNumber].brightness);

	signal(SIGINT, sighandler);
	signal(SIGQUIT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGHUP, sighandler);

	const GLCD::cBitmap * bitmap;
	GLCD::cImage image;
	GLCD::cGLCDFile glcd;
	GLCD::cExtFormatFile extformat;

	int optFile;
	std::string picFile;

	optFile = optind;
	while (optFile < argc && !stopProgramm)
	{
		picFile = argv[optFile++];
		if (glcd.Load(image, picFile) == false) {
			if (extformat.Load(image, picFile) == false) {
				fprintf(stderr, "ERROR: Failed loading file %s\n", picFile.c_str());
				return 8;
			}
		}

		if (delay)
			image.SetDelay(sleepMs);

		lcd->Refresh(true);
		while ((bitmap = image.GetBitmap()) != NULL && !stopProgramm)
		{
//			lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height(), bitmap->LineSize());
			lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height());
			lcd->Refresh(false);

			if (image.Next(0)) // Select next image
			{
				usleep(image.Delay() * 1000);
			}
			else if (endless && argc == (optind + 1)) // Endless and one and only image
			{
				image.First(0);
				usleep(image.Delay() * 1000);
			}
			else 
				break;
		}

		if (optFile < argc || endless)
			usleep(sleepMs * 1000);
		if (optFile >= argc && endless)
			optFile = optind;
	}

	lcd->DeInit();
	delete lcd;

	return 0;
}
