/*
 * GraphLCD graphics library
 *
 * font.c  -  font handling
 *
 * based on graphlcd plugin 0.1.1 for the Video Disc Recorder
 *  (c) 2001-2004 Carsten Siebholz <c.siebholz AT t-online.de>
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>

#include "common.h"
#include "font.h"

#ifdef HAVE_FREETYPE2
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iconv.h>
#endif

namespace GLCD
{

static const char * kFontFileSign = "FNT3";
static const uint32_t kFontHeaderSize = 16;
static const uint32_t kCharHeaderSize = 4;

//#pragma pack(1)
//struct tFontHeader
//{
//    char sign[4];          // = FONTFILE_SIGN
//    unsigned short height; // total height of the font
//    unsigned short ascent; // ascender of the font
//    unsigned short line;   // line height
//    unsigned short reserved;
//    unsigned short space;  // space between characters of a string
//    unsigned short count;  // number of chars in this file
//};
//
//struct tCharHeader
//{
//    unsigned short character;
//    unsigned short width;
//};
//#pragma pack()

cFont::cFont()
{
    Init();
}

cFont::~cFont()
{
    Unload();
}

bool cFont::LoadFNT(const std::string & fileName)
{
    // cleanup if we already had a loaded font
    Unload();

    FILE * fontFile;
    int i;
    uint8_t buffer[10000];
    uint16_t fontHeight;
    uint16_t numChars;
    int maxWidth = 0;

    fontFile = fopen(fileName.c_str(), "rb");
    if (!fontFile)
        return false;

    fread(buffer, kFontHeaderSize, 1, fontFile);
    if (buffer[0] != kFontFileSign[0] ||
        buffer[1] != kFontFileSign[1] ||
        buffer[2] != kFontFileSign[2] ||
        buffer[3] != kFontFileSign[3])
    {
        fclose(fontFile);
        return false;
    }

    fontHeight = buffer[4] | (buffer[5] << 8);
    totalAscent = buffer[6] | (buffer[7] << 8);
    lineHeight = buffer[8] | (buffer[9] << 8);
    spaceBetween = buffer[12] | (buffer[13] << 8);
    numChars = buffer[14] | (buffer[15] << 8);
    for (i = 0; i < numChars; i++)
    {
        uint8_t chdr[kCharHeaderSize];
        uint16_t charWidth;
        uint16_t character;
        fread(chdr, kCharHeaderSize, 1, fontFile);
        character = chdr[0] | (chdr[1] << 8);
        charWidth = chdr[2] | (chdr[3] << 8);
        fread(buffer, fontHeight * ((charWidth + 7) / 8), 1, fontFile);
        if (characters[character])
            delete characters[character];
        characters[character] = new cBitmap(charWidth, fontHeight, buffer);
        if (characters[character]->Width() > maxWidth)
            maxWidth = characters[character]->Width();
    }
    fclose(fontFile);

    totalWidth = maxWidth;
    totalHeight = fontHeight;

    return true;
}

bool cFont::SaveFNT(const std::string & fileName) const
{
    FILE * fontFile;
    uint8_t fhdr[kFontHeaderSize];
    uint8_t chdr[kCharHeaderSize];
    uint16_t numChars;
    int i;

    fontFile = fopen(fileName.c_str(),"w+b");
    if (!fontFile)
    {
        syslog(LOG_ERR, "cFont::SaveFNT(): Cannot open file: %s for writing\n",fileName.c_str());
        return false;
    }

    numChars = 0;
    for (i = 0; i < 256; i++)
    {
        if (characters[i])
        {
            numChars++;
        }
    }

    memcpy(fhdr, kFontFileSign, 4);
    fhdr[4] = (uint8_t) totalHeight;
    fhdr[5] = (uint8_t) (totalHeight >> 8);
    fhdr[6] = (uint8_t) totalAscent;
    fhdr[7] = (uint8_t) (totalAscent >> 8);
    fhdr[8] = (uint8_t) lineHeight;
    fhdr[9] = (uint8_t) (lineHeight >> 8);
    fhdr[10] = 0;
    fhdr[11] = 0;
    fhdr[12] = (uint8_t) spaceBetween;
    fhdr[13] = (uint8_t) (spaceBetween >> 8);
    fhdr[14] = (uint8_t) numChars;
    fhdr[15] = (uint8_t) (numChars >> 8);

    // write font file header
    fwrite(fhdr, kFontHeaderSize, 1, fontFile);

    for (i = 0; i < 256; i++)
    {
        if (characters[i])
        {
            chdr[0] = (uint8_t) i;
            chdr[1] = (uint8_t) (i >> 8);
            chdr[2] = (uint8_t) characters[i]->Width();
            chdr[3] = (uint8_t) (characters[i]->Width() >> 8);
            fwrite(chdr, kCharHeaderSize, 1, fontFile);
            fwrite(characters[i]->Data(), totalHeight * characters[i]->LineSize(), 1, fontFile);
        }
    }

    fclose(fontFile);

    syslog(LOG_DEBUG, "cFont::SaveFNT(): Font file '%s' written successfully\n", fileName.c_str());

    return true;
}

bool cFont::LoadFT2(const std::string & fileName, const std::string & encoding,
                    int size, bool dingBats)
{
    // cleanup if we already had a loaded font
    Unload();
#ifdef HAVE_FREETYPE2
    if (access(fileName.c_str(), F_OK) != 0)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) does not exist!!", fileName.c_str());
        return false;
    }
    // file exists
    FT_Library library;
    FT_Face face;
    FT_GlyphSlot slot;

    int error = FT_Init_FreeType(&library);
    if (error)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: Could not init freetype library");
        return false;
    }
    error = FT_New_Face(library, fileName.c_str(), 0, &face);
    // everything ok?
    if (error == FT_Err_Unknown_File_Format)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) could be opened and read, but it appears that its font format is unsupported", fileName.c_str());
        error = FT_Done_Face(face);
        syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_Face(..) returned (%d)", error);
        error = FT_Done_FreeType(library);
        syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_FreeType(..) returned (%d)", error);
        return false;
    }
    else if (error)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) could not be opened or read, or simply it is broken,\n error code was %x", fileName.c_str(), error);
        error = FT_Done_Face(face);
        syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_Face(..) returned (%d)", error);
        error = FT_Done_FreeType(library);
        syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_FreeType(..) returned (%d)", error);
        return false;
    }

    // set slot
    slot = face->glyph;

    // set Size
    FT_Set_Char_Size(face, 0, size * 64, 0, 0);

    wchar_t utf_buff[256];
    if (dingBats)
    {
/*
        FT_CharMap charmap = 0;
        for (int n = 0; n < face->num_charmaps; n++)
        {
            if (face->charmaps[n]->platform_id == 3 &&
                face->charmaps[n]->encoding_id == 0)
            {
                charmap = face->charmaps[n];
                //break;
            }
        }
        if (charmap)
            syslog(LOG_ERR, "cFont::LoadFT2: platform_id: %d, encoding_id: %d", charmap->platform_id, charmap->encoding_id);
        error = FT_Set_Charmap(_face, charmap);
        if (error)
        {
            syslog(LOG_ERR, "cFont::LoadFT2: FT_Select_Charmap encoding not supported: %d", charmap->encoding_id);
        }
*/
    }
    else
    {
        iconv_t cd;
        if ((cd = iconv_open("WCHAR_T", encoding.c_str())) == (iconv_t) -1)
        {
            syslog(LOG_ERR, "cFont::LoadFT2: Iconv encoding not supported: %s", encoding.c_str());
            error = FT_Done_Face(face);
            syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_Face(..) returned (%d)", error);
            error = FT_Done_FreeType(library);
            syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_FreeType(..) returned (%d)", error);
            return false;
        }
        for (int c = 0; c < 256; c++)
        {
            char char_buff = c;
            wchar_t wchar_buff;
            char * in_buff,* out_buff;
            size_t in_len, out_len, count;

            in_len = 1;
            out_len = 4;
            in_buff = (char *) &char_buff;
            out_buff = (char *) &wchar_buff;
            count = iconv(cd, &in_buff, &in_len, &out_buff, &out_len);
            if ((size_t) -1 == count)
            {
                utf_buff[c] = 0;
            }
            utf_buff[c] = wchar_buff;
        }
        iconv_close(cd);
    }

    // get some global parameters
    totalHeight = (face->size->metrics.ascender >> 6) - (face->size->metrics.descender >> 6);
    totalWidth = face->size->metrics.max_advance >> 6;
    totalAscent = face->size->metrics.ascender >> 6;
    lineHeight = face->size->metrics.height >> 6;
    spaceBetween = 0;
#if 0
    syslog(LOG_DEBUG, "cFont::LoadFT2: totalHeight = %d", totalHeight);
    syslog(LOG_DEBUG, "cFont::LoadFT2: totalWidth = %d", totalWidth);
    syslog(LOG_DEBUG, "cFont::LoadFT2: totalAscent = %d", totalAscent);
    syslog(LOG_DEBUG, "cFont::LoadFT2: lineHeight = %d", lineHeight);
    syslog(LOG_DEBUG, "cFont::LoadFT2: spaceBetween = %d", spaceBetween);
#endif
    // render glyphs for ASCII codes 0 to 255 in our bitmap class
    FT_UInt glyph_index;
    int num_char;

    for (num_char = 0; num_char < 256; num_char++)
    {
        if (dingBats)
        {
            //Get FT char index & load the char
            error = FT_Load_Char(face, num_char, FT_LOAD_DEFAULT);
        }
        else
        {
            //Get FT char index
            glyph_index = FT_Get_Char_Index(face, utf_buff[num_char]);
            //Load the char
            error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        }
        if (error)
        {
            syslog(LOG_ERR, "cFont::LoadFT2: ERROR when calling FT_Load_Glyph: %x", error);
        }

        // convert to a mono bitmap
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
        if (error)
        {
            syslog(LOG_ERR, "cFont::LoadFT2: ERROR when calling FT_Render_Glyph: %x", error);
        }

        // now, fill our pixel data
        cBitmap * charBitmap = new cBitmap(face->glyph->advance.x >> 6, totalHeight);
        charBitmap->Clear();
        unsigned char * bufPtr = face->glyph->bitmap.buffer;
        unsigned char pixel;
        for (int y = 0; y < face->glyph->bitmap.rows; y++)
        {
            for (int x = 0; x < face->glyph->bitmap.width; x++)
            {
                pixel = (bufPtr[x / 8] >> (7 - x % 8)) & 1;
                if (pixel)
                    charBitmap->DrawPixel((face->glyph->metrics.horiBearingX >> 6) + x,
                                          (face->size->metrics.ascender >> 6) - (face->glyph->metrics.horiBearingY >> 6) + y,
                                          GLCD::clrBlack);
            }
            bufPtr += face->glyph->bitmap.pitch;
        }
        SetCharacter((char) num_char, charBitmap);
    }
    error = FT_Done_Face(face);
    if (error)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_Face(..) returned (%d)", error);
    }
    error = FT_Done_FreeType(library);
    if (error)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: FT_Done_FreeType(..) returned (%d)", error);
    }
    return true;
#else
    syslog(LOG_ERR, "cFont::LoadFT2: glcdgraphics was compiled without FreeType2 support!!!");
    return false;
#endif
}

int cFont::Width(char ch) const
{
    if (characters[(unsigned char) ch])
        return characters[(unsigned char) ch]->Width();
    else
        return 0;
}

int cFont::Width(const std::string & str) const
{
    unsigned int i;
    int sum = 0;

    for (i = 0; i < str.length(); i++)
    {
        sum += Width(str[i]);
    }
    if (str.length() > 1)
    {
        sum += spaceBetween * (str.length() - 1);
    }
    return sum;
}

int cFont::Width(const std::string & str, unsigned int len) const
{
    unsigned int i;
    int sum = 0;

    for (i = 0; i < str.length() && i < len; i++)
    {
        sum += Width(str[i]);
    }
    if (std::min(str.length(), (size_t) len) > 1)
    {
        sum += spaceBetween * (std::min(str.length(), (size_t) len) - 1);
    }
    return sum;
}

int cFont::Height(char ch) const
{
    if (characters[(unsigned char) ch])
        return characters[(unsigned char) ch]->Height();
    else
        return 0;
}

int cFont::Height(const std::string & str) const
{
    unsigned int i;
    int sum = 0;

    for (i = 0; i < str.length(); i++)
        sum = std::max(sum, Height(str[i]));
    return sum;
}

int cFont::Height(const std::string & str, unsigned int len) const
{
    unsigned int i;
    int sum = 0;

    for (i = 0; i < str.length() && i < len; i++)
        sum = std::max(sum, Height(str[i]));
    return sum;
}

const cBitmap * cFont::GetCharacter(char ch) const
{
    return characters[(unsigned char) ch];
}

void cFont::SetCharacter(char ch, cBitmap * bitmapChar)
{
    // adjust maxwidth if necessary
    if (totalWidth < bitmapChar->Width())
        totalWidth = bitmapChar->Width();

    // delete if already allocated
    if (characters[(unsigned char) ch])
        delete characters[(unsigned char) ch];

    // store new character
    characters[(unsigned char) ch] = bitmapChar;
}

void cFont::Init()
{
    totalWidth = 0;
    totalHeight = 0;
    totalAscent = 0;
    spaceBetween = 0;
    lineHeight = 0;
    for (int i = 0; i < 256; i++)
    {
        characters[i] = NULL;
    }
}

void cFont::Unload()
{
    // cleanup
    for (int i = 0; i < 256; i++)
    {
        if (characters[i])
        {
            delete characters[i];
        }
    }
    // re-init
    Init();
}

void cFont::WrapText(int Width, int Height, std::string & Text,
                     std::vector <std::string> & Lines, int * ActualWidth) const
{
    int maxLines;
    int lineCount;
    int textWidth;
    std::string::size_type start;
    std::string::size_type pos;
    std::string::size_type posLast;

    Lines.clear();
    maxLines = 2000;
    if (Height > 0)
    {
        maxLines = Height / LineHeight();
        if (maxLines == 0)
            maxLines = 1;
    }
    lineCount = 0;

    pos = 0;
    start = 0;
    posLast = 0;
    textWidth = 0;
    while (pos < Text.length() && (Height == 0 || lineCount < maxLines))
    {
        if (Text[pos] == '\n')
        {
            Lines.push_back(trim(Text.substr(start, pos - start)));
            start = pos + 1;
            posLast = pos + 1;
            textWidth = 0;
            lineCount++;
        }
        else if (textWidth > Width && (lineCount + 1) < maxLines)
        {
            if (posLast > start)
            {
                Lines.push_back(trim(Text.substr(start, posLast - start)));
                start = posLast + 1;
                posLast = start;
                textWidth = this->Width(Text.substr(start, pos - start + 1)) + spaceBetween;
            }
            else
            {
                Lines.push_back(trim(Text.substr(start, pos - start)));
                start = pos + 1;
                posLast = start;
                textWidth = this->Width(Text[pos]) + spaceBetween;
            }
            lineCount++;
        }
        else if (Text[pos] == ' ')
        {
            posLast = pos;
            textWidth += this->Width(Text[pos]) + spaceBetween;
        }
        else
        {
            textWidth += this->Width(Text[pos]) + spaceBetween;
        }
        pos++;
    }

    if (Height == 0 || lineCount < maxLines)
    {
        if (textWidth > Width && (lineCount + 1) < maxLines)
        {
            if (posLast > start)
            {
                Lines.push_back(trim(Text.substr(start, posLast - start)));
                start = posLast + 1;
                posLast = start;
                textWidth = this->Width(Text.substr(start, pos - start + 1)) + spaceBetween;
            }
            else
            {
                Lines.push_back(trim(Text.substr(start, pos - start)));
                start = pos + 1;
                posLast = start;
                textWidth = this->Width(Text[pos]) + spaceBetween;
            }
            lineCount++;
        }
        if (pos > start)
        {
            Lines.push_back(trim(Text.substr(start)));
            lineCount++;
        }
        textWidth = 0;
        for (int i = 0; i < lineCount; i++)
            textWidth = std::max(textWidth, this->Width(Lines[i]));
        textWidth = std::min(textWidth, Width);
    }
    else
        textWidth = Width;
    if (ActualWidth)
        *ActualWidth = textWidth;
}

} // end of namespace
