/**
 *  convpic.c  -  a tool to convert images to
 *                own proprietary format of the logos and pictures
 *                for graphlcd plugin
 *
 *  (C) 2004 Andreas Brachold <vdr04 AT deltab de>
 *  (C) 2001-2003 by Carsten Siebholz <c.siebholz AT t-online.de>
 *  (C) 2013         Wolfgang Astleitner <mrwastl AT users sourceforge net>
 **/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program;                                              *
 *   if not, write to the Free Software Foundation, Inc.,                  *
 *   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA               *
 *                                                                         *
 ***************************************************************************/

#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/image.h>
#include <glcdgraphics/imagefile.h>
#include <glcdgraphics/glcd.h>
#include <glcdgraphics/pbm.h>

static const char *prgname = "convpic";
static const char *VERSION = "0.1.3";

unsigned int delay = 250;


void usage(void) {
    fprintf(stdout, "\n");
    fprintf(stdout, "%s v%s\n", prgname, VERSION);
    fprintf(stdout, "%s is a tool to convert images to a simple format (*.glcd)\n", prgname);
    fprintf(stdout, "        that is used by the graphlcd plugin for VDR.\n\n");
    fprintf(stdout, "  Usage: %s [-n] -i file[s...] -o outfile \n\n", prgname);
    fprintf(stdout, "  -n  --invert      inverts the output (default: none)\n");
    fprintf(stdout, "  -i  --infile      specifies the name of the input file[s]\n");
    fprintf(stdout, "  -o  --outfile     specifies the name of the output file\n");
    fprintf(stdout, "  -d  --delay       specifies the delay between multiple images [Default: %d ms] \n",delay);
    fprintf(stdout, "\n" );
    fprintf(stdout, "  example: %s -i vdr-logo.bmp -o vdr-logo.glcd \n", prgname );
}


int main(int argc, char *argv[]) {
    std::string  inFile = "";
    std::string  outFile = "";
    std::string  inExtension = "";
    std::string  outExtension = "";
    GLCD::cImage  image;
    GLCD::cImage  nextImage;
    bool  bError = false;
    bool  bInvert = false;
    bool  bDelay = false;

    unsigned int image_w = 0;
    unsigned int image_h = 0;


    static struct option long_options[] =
    {
        {"invert",         no_argument, NULL, 'n'},
        {"infile",   required_argument, NULL, 'i'},
        {"outfile",  required_argument, NULL, 'o'},
        {"delay",    required_argument, NULL, 'd'},
        { NULL}
    };

    int c, option_index = 0;
    while ((c=getopt_long(argc,argv,"ni:o:d:",long_options, &option_index))!=-1) {
        switch (c) {
            case 'n':
                bInvert = true;
                break;

            case 'i':
                inFile = optarg;
                break;

            case 'o':
                outFile = optarg;
                break;

            case 'd':
                delay = atoi(optarg);
                bDelay = true;
                if (delay < 10)
                {
                    fprintf(stderr, "WARNING: Delay is too short, minimum delay is 10 ms\n");
                    delay = 10;
                }
                break;

            default:
                usage();
                return 1;
        }
    }

    if (inFile.length() == 0) {
        fprintf(stderr, "ERROR: You have to specify the infile (-i filename)\n");
        bError = true;
    }

    inExtension = GLCD::cImage::GetFilenameExtension(inFile);

    if (outFile.length() == 0) {
      outFile = inFile.substr(0, inFile.length() - inExtension.length() - 1);
      if (inExtension == "GLCD") {
        outFile += ".pbm";
        outExtension = "PBM";
      } else {
        outFile += ".glcd";
        outExtension = "GLCD";
      }
      fprintf(stderr, "WARNING: No output filename given, will use '%s'\n", outFile.c_str());
    }

    if (outExtension.length() == 0) {
      outExtension = GLCD::cImage::GetFilenameExtension(outFile);
    }

    if ( !(outExtension == "GLCD" || outExtension == "PBM") ) {
        fprintf(stderr, "ERROR: Unsupported format for outfile ('%s')\n", outExtension.c_str());
        bError = true;
    }

    if ( inExtension != "GLCD" && outExtension != "GLCD" ) {
        fprintf(stderr, "ERROR: Either infile or outfile needs to be a GLCD file\n");
        bError = true;
    }

    if ( outExtension != "GLCD" && outExtension != "PBM" ) {
        fprintf(stderr, "ERROR: outfile needs to be either GLCD or PBM\n");
        bError = true;
    }

    if (bError) {
        usage();
        return 1;
    }

    // Load Picture
    fprintf(stdout, "loading %s\n", inFile.c_str());
    if (GLCD::cImage::LoadImage(image, inFile) == false) {
      fprintf(stderr, "ERROR: Failed loading file %s\n", inFile.c_str());
      bError = true;
    }

    /* if more than one input image: following images must match width and height of first image */
    image_w = image.GetBitmap(0)->Width();
    image_h = image.GetBitmap(0)->Height();

    if (!bError) {
        GLCD::cImageFile * outImage = NULL;
        // Load more in files
        while (optind < argc && !bError) {
            inFile = argv[optind++];

            fprintf(stdout, "loading %s\n", inFile.c_str());
            if (GLCD::cImage::LoadImage(nextImage, inFile) == false) {
              fprintf(stderr, "ERROR: Failed loading file '%s', ignoring it ...\n", inFile.c_str());
            } else {
              unsigned int nim_w = nextImage.GetBitmap(0)->Width();
              unsigned int nim_h = nextImage.GetBitmap(0)->Height();
              if ( nim_w != image_w || nim_h != image_h) {
                fprintf(stderr, "ERROR: Image '%s' is not matching dimensions of first image, ignoring it ...\n", inFile.c_str());
              } else {
                uint16_t i;
                for (i = 0; i < nextImage.Count(); i++) {
                    image.AddBitmap(new GLCD::cBitmap(*nextImage.GetBitmap(i)));
                }
              }
            }
        }
        if (bDelay)
            image.SetDelay(delay);
        if (bInvert) {
            uint16_t i;
            for (i = 0; i < image.Count(); i++) {
                image.GetBitmap(i)->Invert();
            }
        }
        if (outExtension == "PBM") {
          outImage = new GLCD::cPBMFile();
        } else {
          outImage = new GLCD::cGLCDFile();
        }
        fprintf(stdout, "saving %s\n", outFile.c_str());
        bError = !outImage->Save(image, outFile);
    }
    if (bError) {
        return 4;
    }

    fprintf(stdout, "conversion compeleted successfully.\n\n");

    return 0;
}
