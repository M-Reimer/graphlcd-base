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
 * (c) 2010-2013 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *               Andreas 'randy' Weinberger
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <dlfcn.h>
#include <unistd.h>
#include <signal.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/image.h>

#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>

static const char *prgname = "showpic";
static const char *version = "0.1.3";

static const int kDefaultSleepMs = 100;
static const char * kDefaultConfigFile = "/etc/graphlcd.conf";

static volatile bool stopProgramm = false;

static void sighandler(int signal) {
  switch (signal) {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      stopProgramm = true;
  }
}

void usage() {
  fprintf(stdout, "\n");
  fprintf(stdout, "%s v%s\n", prgname, version);
  fprintf(stdout, "%s is a tool to show an image on an LCD.\n", prgname);
  fprintf(stdout, "The image format must be supported by libglcdgraphics.\n\n");
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
  fprintf(stdout, "  -S  --scale       scale algorithm to be used\n");
  fprintf(stdout, "                    0: don't scale (default)\n");
  fprintf(stdout, "                    1: fit to display if larger but not if smaller (keeping aspect ratio) \n");
  fprintf(stdout, "                    2: fit to display if larger or smaller (keeping aspect ratio)\n");
  fprintf(stdout, "                    3: fill display (ignoring aspect ratio)\n");
  fprintf(stdout, "  -C  --center      center image (default: no)\n");
  fprintf(stdout, "\n" );
  fprintf(stdout, "  examples: %s -c /etc/graphlcd.conf vdr-logo.glcd\n", prgname);
  fprintf(stdout, "            %s -c /etc/graphlcd.conf -d somedisplay -u -i vdr-animation.glcd\n", prgname);
  fprintf(stdout, "\n" );
}

int main(int argc, char *argv[]) {
  static struct option long_options[] =
  {
    {"config",     required_argument, NULL, 'c'},
    {"display",    required_argument, NULL, 'd'},
    {"sleep",      required_argument, NULL, 's'},
    {"endless",          no_argument, NULL, 'e'},
    {"upsidedown",       no_argument, NULL, 'u'},
    {"invert",           no_argument, NULL, 'i'},
    {"brightness", required_argument, NULL, 'b'},
    {"scale",      required_argument, NULL, 'S'},
    {"center",           no_argument, NULL, 'C'},
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
  int scaleAlgo = 0;
  bool center = false;

  int c, option_index = 0;
  while ((c = getopt_long(argc, argv, "c:d:s:S:euib:C", long_options, &option_index)) != -1) {
    switch(c) {
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

      case 'S':
        scaleAlgo = atoi(optarg);
        if (scaleAlgo < 0 || scaleAlgo > 3) scaleAlgo = 0;
        break;

      case 'e':
        endless = true;
        break;

      case 'b':
        brightness = atoi(optarg);
        if (brightness < 0) brightness = 0;
        if (brightness > 100) brightness = 100;
        break;

      case 'C':
        center = true;
        break;

      default:
        usage();
        return 1;
    }
  }

  if (configName.length() == 0) {
    configName = kDefaultConfigFile;
    fprintf(stderr, "Error: No config file specified, using default (%s).\n", configName.c_str());
  }

  if (GLCD::Config.Load(configName) == false) {
    fprintf(stderr, "Error loading config file!\n");
    return 2;
  }
  if (GLCD::Config.driverConfigs.size() > 0) {
    if (displayName.length() > 0) {
      for (displayNumber = 0; displayNumber < GLCD::Config.driverConfigs.size(); displayNumber++) {
        if (GLCD::Config.driverConfigs[displayNumber].name == displayName)
          break;
      }
      if (displayNumber == GLCD::Config.driverConfigs.size()) {
        fprintf(stderr, "ERROR: Specified display %s not found in config file!\n", displayName.c_str());
        return 3;
      }
    } else {
      fprintf(stderr, "WARNING: No display specified, using first one.\n");
      displayNumber = 0;
    }
  } else {
    fprintf(stderr, "ERROR: No displays specified in config file!\n");
    return 4;
  }

  if (optind == argc) {
    usage();
    fprintf(stderr, "ERROR: You have to specify the image\n");
    return 5;
  }

  GLCD::Config.driverConfigs[displayNumber].upsideDown ^= upsideDown;
  GLCD::Config.driverConfigs[displayNumber].invert ^= invert;
  if (brightness != -1)
    GLCD::Config.driverConfigs[displayNumber].brightness = brightness;
  GLCD::cDriver * lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
  if (!lcd) {
    fprintf(stderr, "ERROR: Failed creating display object %s\n", displayName.c_str());
    return 6;
  }
  if (lcd->Init() != 0) {
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

  GLCD::cBitmap * buffer = new GLCD::cBitmap(lcd->Width(), lcd->Height());

  int optFile;
  std::string picFile;

  optFile = optind;
  while (optFile < argc && !stopProgramm) {
    picFile = argv[optFile++];
    if (GLCD::cImage::LoadImage(image, picFile) == false) {
      fprintf(stderr, "ERROR: Failed loading file %s\n", picFile.c_str());
      return 8;
    }

    if (scaleAlgo > 0) {
      uint16_t scalew = 0;
      uint16_t scaleh = 0;
      uint16_t imagew = image.Width();
      uint16_t imageh = image.Height();

      switch (scaleAlgo) {
        case 1:
        case 2:
          if (imagew > lcd->Width() || imageh > lcd->Height()) {
            if ((double)imagew / (double)lcd->Width() > (double)imageh / (double)lcd->Height())
              scalew = lcd->Width();
            else
              scaleh = lcd->Height();
          } else if (scaleAlgo == 2 && imagew < lcd->Width() && imageh < lcd->Height()) {
            if ((double)imagew / (double)lcd->Width() > (double)imageh / (double)lcd->Height())
              scalew = lcd->Width();
            else
              scaleh = lcd->Height();
          }
          break;
        default: /* 3 */
          scalew = lcd->Width();
          scaleh = lcd->Height();
      }
      image.Scale(scalew, scaleh, false);
    }

    if (delay)
      image.SetDelay(sleepMs);


    uint16_t xstart = 0;
    uint16_t ystart = 0;

    if (center) {
      if ((unsigned int)(lcd->Width()) > image.Width())
        xstart = (lcd->Width() - image.Width()) >> 1;
      if ((unsigned int)(lcd->Height()) > image.Height())
        ystart = (lcd->Height() - image.Height()) >> 1;
    }

    lcd->Refresh(true);
    while ((bitmap = image.GetBitmap()) != NULL && !stopProgramm) {
      buffer->Clear();
      buffer->DrawBitmap(xstart, ystart, *bitmap);
      lcd->SetScreen(buffer->Data(), buffer->Width(), buffer->Height());
      lcd->Refresh(false);

      if (image.Next(0)) { // Select next image
        usleep(image.Delay() * 1000);
      } else if (endless && argc == (optind + 1)) { // Endless and one and only image
        image.First(0);
        usleep(image.Delay() * 1000);
      } else {
        break;
      }
    }

    if (optFile < argc || endless)
      usleep(sleepMs * 1000);
    if (optFile >= argc && endless)
      optFile = optind;
  }

  delete buffer;
  lcd->DeInit();
  delete lcd;

  return 0;
}
