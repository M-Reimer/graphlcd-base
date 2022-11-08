/*
 * GraphLCD tool genfont
 *
 * genfont.c  -  a tool to create *.fnt files for use with the GraphLCD
 *               graphics library
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *               Andreas 'randy' Weinberger
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>
#include <glcdgraphics/image.h>
#include <glcdgraphics/pbm.h>

static const char *prgname = "genfont";
static const char *version = "0.0.2";

void usage(void)
{
    fprintf(stdout, "\n");
    fprintf(stdout, "%s v%s\n", prgname, version);
    fprintf(stdout, "%s is a tool to create *.fnt files that are used by the\n", prgname);
    fprintf(stdout, "        graphlcd plugin for VDR.\n\n");
    fprintf(stdout, "  Usage: %s -f <format> -i infile -o outfile -s size\n\n", prgname);
    fprintf(stdout, "  -f  --format  specifies the format of the output files:\n");
    fprintf(stdout, "                  0 - fnt (default)\n");
    fprintf(stdout, "                  1 - pbm & desc\n");
    fprintf(stdout, "  -i  --input   specifies the name of the input font file (*.ttf)\n");
    fprintf(stdout, "  -o  --output  specifies the base name of the output files\n");
    fprintf(stdout, "  -s  --size    font size of the generated font file\n");
    fprintf(stdout, "\n" );
    fprintf(stdout, "  example: %s -i verdana.ttf -o verdana20 -s 20\n", prgname);
    fprintf(stdout, "           %s -f 1 -i verdana.ttf -o verdana20 -s 20\n", prgname);
}


int main(int argc, char *argv[])
{
    static struct option long_options[] =
    {
        { "format", required_argument, NULL, 'f'},
        { "input",  required_argument, NULL, 'i'},
        { "output", required_argument, NULL, 'o'},
        { "size",   required_argument, NULL, 's'},
        { NULL}
    };

    int c;
    int option_index = 0;
    int format = 0;
    std::string inputFontFile = "";
    std::string outputName = "";
    int size = 30;

    while ((c = getopt_long(argc, argv, "f:i:o:s:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
            case 'f':
                format = atoi(optarg);
                break;

            case 'i':
                inputFontFile = optarg;
                break;

            case 'o':
                outputName = optarg;
                break;

            case 's':
                size = atoi(optarg);
                break;

            default:
                usage();
                break;
        }
    }
    if (format > 1)
    {
        usage();
        return 1;
    }
    if (inputFontFile == "")
    {
        usage();
        return 1;
    }
    if (outputName == "")
    {
        outputName = inputFontFile;
    }

    GLCD::cFont font;
    if (!font.LoadFT2(inputFontFile, "iso-8859-1", size, false))
    {
        return 1;
    }

    GLCD::cBitmap * bitmap = NULL;
    std::string fileName;
    FILE * descFile = NULL;
    int posX = 0;
    int posY = 0;

    if (format == 0)
    {
        fileName = outputName + ".fnt";
        if (!font.SaveFNT(fileName))
        {
            return 1;
        }
    }
    else
    {
        fileName = outputName + ".desc";
        descFile = fopen(fileName.c_str(), "wb");
        if (!descFile)
        {
            fprintf(stderr, "Cannot open file: %s\n", fileName.c_str());
            return 1;
        }
        bitmap = new GLCD::cBitmap(32 * font.TotalWidth(), 8 * font.TotalHeight());
        bitmap->Clear();
        fprintf(descFile, "version:1\n");
        fprintf(descFile, "fontheight:%d\n", font.TotalHeight());
        fprintf(descFile, "fontascent:%d\n", font.TotalAscent());
        fprintf(descFile, "lineheight:%d\n", font.LineHeight());
        fprintf(descFile, "spacebetween:%d\n", 0);
        fprintf(descFile, "spacewidth:%d\n", 0);

        for (uint32_t i = 0; i < 256; i++)
        {
            const GLCD::cBitmap * charBitmap = font.GetCharacter(i);

            if (charBitmap == NULL)
                continue;

            bitmap->DrawBitmap(posX, posY, *charBitmap);
            fprintf(descFile, "%d %d ", posX, i);
            posX += charBitmap->Width();
            if ((i % 32) == 31)
            {
                fprintf(descFile, "%d\n", posX);
                posY += font.TotalHeight();
                posX = 0;
            }
        }

        if (posX > 0) // write last end marker
            fprintf(descFile, "%d\n", posX);
        fileName = outputName + ".pbm";

        GLCD::cPBMFile pbm;
        GLCD::cImage* image = new GLCD::cImage();

        if (!image)
            return 3;

        image->AddBitmap(bitmap);

        if (pbm.Save(*image, fileName) == false) {
            fprintf(stderr, "Cannot save file: %s\n",fileName.c_str());
            delete image;
            return 2;
        }

        fclose(descFile);
        delete image;
    }

    fprintf(stdout, "Font successfully generated.\n");

    return 0;
}
