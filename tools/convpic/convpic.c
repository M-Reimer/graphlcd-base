/**
 *  convpic.c  -  a tool to convert images to
 *                own proprietary format of the logos and pictures
 *                for graphlcd plugin
 *
 *  (C) 2004 Andreas Brachold <vdr04 AT deltab de>
 *  (C) 2001-2003 by Carsten Siebholz <c.siebholz AT t-online.de>
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

#include "bmp.h"
#include "tiff.h"
#include "tuxbox.h"

static const char *prgname = "convpic";
static const char *VERSION = "0.1.1";

unsigned int delay = 250;


enum ePicFormat
{
    pfUndefined,
    pfTIFF,
    pfBMP,
    pfGLCD,
    pfPBM,
    pfTUXBOX
};

void usage(void);

ePicFormat getFormat(const char* szFile)
{
    static const struct tagformats {const char* szExt; ePicFormat picformat;} formats[] =
    {
        {".tiff", pfTIFF  },
        {".tif",  pfTIFF  },
        {".bmp",  pfBMP   },
        {".glcd", pfGLCD  },
        {".pbm",  pfPBM   },
        {".ani",  pfTUXBOX}
    };
    ePicFormat pf = pfUndefined;

    if (szFile)
    {
        for (int i = strlen(szFile) - 1; i >= 0; i--)
        {
            if (*(szFile+i) == '.' && strlen(szFile + i + 1))
            {
                for (unsigned int n = 0; n < sizeof(formats)/sizeof(*formats); n++)
                {
                    if (!strcasecmp((szFile+i), formats[n].szExt))
                    {
                        return formats[n].picformat;
                    }
                }
            }
        }
    }
    return pf;
}

GLCD::cImageFile * GetFileTranslator(ePicFormat Format)
{
    switch (Format)
    {
        case pfGLCD:
            return new GLCD::cGLCDFile();

        case pfPBM:
            return new GLCD::cPBMFile();

        case pfBMP:
            return new cBMPFile();

        case pfTIFF:
            return new cTIFFFile();

        case pfTUXBOX:
            return new cTuxBoxFile();

        default:
            return NULL;
    }

}

int main(int argc, char *argv[]) {
    ePicFormat  inFormat = pfUndefined;
    ePicFormat  outFormat = pfUndefined;
    std::string  inFile = "";
    std::string  outFile = "";
    GLCD::cImage  image;
    GLCD::cImage  nextImage;
    GLCD::cImageFile *  pInBitmap = NULL;
    GLCD::cImageFile *  pOutBitmap = NULL;
    bool  bError = false;
    bool  bInvert = false;
    bool  bDelay = false;


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
                    fprintf(stderr, "Warning: You have specify a to short delay, minimum are 10 ms\n");
                    delay = 10;
                }
                break;

            default:
                return 1;
        }
    }

    if (inFile.length() == 0)
    {
        fprintf(stderr, "ERROR: You have to specify the infile (-i filename)\n");
        bError = true;
    }

    if (pfUndefined == (inFormat = getFormat(inFile.c_str())))
    {
        fprintf(stderr, "ERROR: You have to specify a correct extension for the %s\n", inFile.c_str());
        bError = true;
    }

    if (outFile.length() == 0)
    {
        fprintf(stderr, "ERROR: You have to specify the outfile (-o filename)\n");
        bError = true;
    }

    if (pfUndefined == (outFormat = getFormat(outFile.c_str())))
    {
        fprintf(stderr, "ERROR: You have to specify a correct extension for the %s \n", outFile.c_str());
        bError = true;
    }

    if (bError)
    {
        usage();
        return 1;
    }


    pInBitmap = GetFileTranslator(inFormat);
    if (!pInBitmap)
        return 2;

    pOutBitmap = GetFileTranslator(outFormat);
    if (!pOutBitmap)
        return 3;

    // Load Picture
    fprintf(stdout, "loading %s\n", inFile.c_str());
    bError = !pInBitmap->Load(image, inFile);
    if (!bError)
    {
        // Load more in files
        while (optind < argc && !bError)
        {
            inFile = argv[optind++];
            inFormat = getFormat(inFile.c_str());
            if (inFormat == pfUndefined)
            {
                fprintf(stderr, "ERROR: You have to specify a correct extension for the %s\n", inFile.c_str());
                bError = true;
                break;
            }
            pInBitmap = GetFileTranslator(inFormat);
            if (!pInBitmap)
                break;

            fprintf(stdout, "loading %s\n", inFile.c_str());
            if (pInBitmap->Load(nextImage, inFile))
            {
                uint16_t i;
                for (i = 0; i < nextImage.Count(); i++)
                {
                    image.AddBitmap(new GLCD::cBitmap(*nextImage.GetBitmap(i)));
                }
            }
        }
        if (bDelay)
            image.SetDelay(delay);
        if (bInvert)
        {
            uint16_t i;
            for (i = 0; i < image.Count(); i++)
            {
                image.GetBitmap(i)->Invert();
            }
        }
        fprintf(stdout, "saving %s\n", outFile.c_str());
        bError = !pOutBitmap->Save(image, outFile);
    }
    if (bError) {
        return 4;
    }

    fprintf(stdout, "conversion compeleted successfully.\n\n");

    return 0;
}

void usage(void)
{
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
