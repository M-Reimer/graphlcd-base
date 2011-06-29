/*
 * GraphLCD tool showtext
 *
 * showtext.c  -  a tool to show a text on a LCD
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
#include <unistd.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>
#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>

static const char *prgname = "showtext";
static const char *version = "0.0.3";

static const char * kDefaultConfigFile = "/etc/graphlcd.conf";

static const int kFontFNT = 0;
static const int kFontFT2 = 1;

void usage()
{
	fprintf(stdout, "\n");
	fprintf(stdout, "%s v%s\n", prgname, version);
	fprintf(stdout, "%s is a tool to show a text on an LCD.\n", prgname);
	fprintf(stdout, "\n");
	fprintf(stdout, "  Usage: %s [-c CONFIGFILE] [-d DISPLAY] [-f FONT] [-x XPOS] [-y YPOS] [-uib] text [more text]\n\n", prgname);
	fprintf(stdout, "  -c  --config      specifies the location of the config file\n");
	fprintf(stdout, "                    (default: /etc/graphlcd.conf)\n");
	fprintf(stdout, "  -d  --display     specifies the output display (default is the first one)\n");
	fprintf(stdout, "  -f  --font        specifies the font that is used for the text:\n");
	fprintf(stdout, "                    fnt:/path/to/font.fnt for GraphLCD font format\n");
	fprintf(stdout, "                    ft2:/path/to/font.ttf:size for FreeType2 supported fonts\n");
	fprintf(stdout, "  -e  --encoding    specifies the encoding that is used for the text\n");
	fprintf(stdout, "                    (p.e. iso8859-1)\n");
	fprintf(stdout, "  -x  --xpos        specifies the x-position where the text starts\n");
	fprintf(stdout, "  -y  --ypos        specifies the y-position where the text starts\n");
	fprintf(stdout, "  -u  --upsidedown  rotates the output by 180 degrees (default: no)\n");
	fprintf(stdout, "  -i  --invert      inverts the output (default: no)\n");
	fprintf(stdout, "  -b  --brightness  set brightness for display if driver support it [%%]\n");
	fprintf(stdout, "                    (default: config file value)\n");
	fprintf(stdout, "\n" );
	fprintf(stdout, "  example: %s -c /etc/graphlcd.conf -f fnt:f17.fnt \"Line1\" \"Line2\"\n", prgname);
	fprintf(stdout, "\n" );
}

int main(int argc, char *argv[])
{
	static struct option long_options[] =
	{
		{"config",     required_argument, NULL, 'c'},
		{"display",    required_argument, NULL, 'd'},
		{"font",       required_argument, NULL, 'f'},
		{"encoding",   required_argument, NULL, 'e'},
		{"upsidedown",       no_argument, NULL, 'u'},
		{"invert",           no_argument, NULL, 'i'},
		{"brightness", required_argument, NULL, 'b'},
		{"xpos",       required_argument, NULL, 'x'},
		{"ypos",       required_argument, NULL, 'y'},
		{NULL}
	};

	std::string configName = "";
	std::string displayName = "";
	std::string fontName;
	std::string fontFileName = "";
	std::string encoding = "";
	int fontType;
	int fontSize = 16;
	bool upsideDown = false;
	bool invert = false;
	int brightness = -1;
	int x = 0;
	int y = 0;
	unsigned int displayNumber = 0;

	int c, option_index = 0;
	while ((c = getopt_long(argc, argv, "c:d:f:e:uib:x:y:", long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 'c':
				configName = optarg;
				break;

			case 'd':
				displayName = optarg;
				break;

			case 'f':
				fontName = optarg;
				break;

			case 'e':
				encoding = optarg;
				break;

			case 'u':
				upsideDown = true;
				break;

			case 'i':
				invert = true;
				break;

			case 'b':
				brightness = atoi(optarg);
				if (brightness < 0) brightness = 0;
				if (brightness > 100) brightness = 100;
				break;

			case 'x':
				x = atoi(optarg);
				break;

			case 'y':
				y = atoi(optarg);
				break;

			default:
				usage();
				return 1;
		}
	}

	if (configName.length() == 0)
	{
		configName = kDefaultConfigFile;
		fprintf(stdout, "WARNING: No config file specified, using default (%s).\n", configName.c_str());
	}

	if (GLCD::Config.Load(configName) == false)
	{
		fprintf(stderr, "Error loading config file!\n");
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
				fprintf(stderr, "ERROR: Specified display %s not found in config file!\n", displayName.c_str());
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
		fprintf(stderr, "ERROR: No displays specified in config file!\n");
		return 4;
	}

	if (encoding.length() == 0)
	{
		encoding = "iso8859-1";
		fprintf(stdout, "WARNING: No encoding specified, using default (%s).\n", encoding.c_str());
	}

	if (fontName.length() == 0)
	{
		fprintf(stderr, "Error: No font specified!\n");
		return 5;
	}
	if (fontName.find("fnt:") == 0)
	{
		fontType = kFontFNT;
		fontFileName = fontName.substr(4);
	}
	else if (fontName.find("ft2:") == 0)
	{
		fontType = kFontFT2;
		std::string::size_type pos = fontName.find(":", 4);
		if (pos == std::string::npos)
		{
			fontSize = 16;
			fprintf(stdout, "WARNING: No font size specified, using default (%d).\n", fontSize);
			fontFileName = fontName.substr(4);
		}
		else
		{
			std::string tmp = fontName.substr(pos + 1);
			fontSize = atoi(tmp.c_str());
			fontFileName = fontName.substr(4, pos - 4);
		}
	}
	else
	{
		fprintf(stderr, "Error: Unknown font type!\n");
		return 6;
	}
	
	if (optind == argc)
	{
		usage();
		fprintf(stderr, "ERROR: You have to specify a text\n");
		return 7;
	}

	GLCD::Config.driverConfigs[displayNumber].upsideDown ^= upsideDown;
	GLCD::Config.driverConfigs[displayNumber].invert ^= invert;
	if (brightness != -1)
		GLCD::Config.driverConfigs[displayNumber].brightness = brightness;

	GLCD::cFont font;
	if (fontType == kFontFNT)
	{
		if (font.LoadFNT(fontFileName) == false)
		{
			fprintf(stderr, "ERROR: Failed loading font file %s\n", fontFileName.c_str());
			return 8;
		}
	}
	else
	{
		if (font.LoadFT2(fontFileName, encoding, fontSize) == false)
		{
			fprintf(stderr, "ERROR: Failed loading font file %s\n", fontFileName.c_str());
			return 8;
		}
	}
	GLCD::cDriver * lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
	if (!lcd)
	{
		fprintf(stderr, "ERROR: Failed creating display object %s\n", displayName.c_str());
		return 9;
	}
	if (lcd->Init() != 0)
	{
		fprintf(stderr, "ERROR: Failed initializing display %s\n", displayName.c_str());
		delete lcd;
		return 10;
	}

	GLCD::cBitmap * bitmap = new GLCD::cBitmap(lcd->Width(), lcd->Height());
	int optText;
	std::string text;

	bitmap->Clear();
	optText = optind;
	while (optText < argc)
	{
		text = argv[optText];
		optText++;

		bitmap->DrawText(x, y, bitmap->Width() - 1, text, &font);
		y += font.LineHeight();
	}
//	lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height(), bitmap->LineSize());
	lcd->SetScreen(bitmap->Data(), bitmap->Width(), bitmap->Height());
	lcd->Refresh(true);

	lcd->DeInit();
	delete lcd;

	return 0;
}
