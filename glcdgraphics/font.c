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

#include <cstring>
#include <algorithm>
#include <cstring>

#include "common.h"
#include "font.h"

#ifdef HAVE_FREETYPE2
#include <ft2build.h>
#include FT_FREETYPE_H
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

#ifdef HAVE_FREETYPE2

class cBitmapCache
{
private:
protected:
    cBitmapCache *next;  // next  bitmap
    cBitmap      *ptr;
    int          charcode;
public:
    cBitmapCache();
    ~cBitmapCache();

    void PushBack(int ch, cBitmap *bitmap);
    cBitmap *GetBitmap(int ch) const;
};

cBitmapCache::cBitmapCache()
:   next(NULL),
    ptr(NULL),
    charcode(0)
{
}

cBitmapCache::~cBitmapCache()
{
    delete ptr;
    delete next;
}

void cBitmapCache::PushBack(int ch, cBitmap *bitmap)
{
    if (!ptr)
    {
        charcode = ch;
        ptr = bitmap;
    }
    else if (!next)
    {
        next = new cBitmapCache();
        next->ptr = bitmap;
        next->charcode = ch;
    } else
        next->PushBack(ch, bitmap);
}

cBitmap *cBitmapCache::GetBitmap(int ch) const
{
    if (ptr && charcode==ch)
        return ptr;
    else if (next)
        return next->GetBitmap(ch);
    else
        return NULL;
}

#endif

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
    loadedFontType = lftFNT; //original fonts

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
    loadedFontType = lftFT2; // ft2 fonts
#ifdef HAVE_FREETYPE2
    if (access(fileName.c_str(), F_OK) != 0)
    {
        syslog(LOG_ERR, "cFont::LoadFT2: Font file (%s) does not exist!!", fileName.c_str());
        return false;
    }
    // file exists
    FT_Library library;
    FT_Face face;

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

    // set Size
    FT_Set_Char_Size(face, 0, size * 64, 0, 0);

    // get some global parameters
    SetTotalHeight( (face->size->metrics.ascender >> 6) - (face->size->metrics.descender >> 6) );
    SetTotalWidth ( face->size->metrics.max_advance >> 6 );
    SetTotalAscent( face->size->metrics.ascender >> 6 );
    SetLineHeight ( face->size->metrics.height >> 6 );
    SetSpaceBetween( 0 );

    ft2_library = library;
    ft2_face = face;

    characters_cache=new cBitmapCache(); 
    return true;
#else
    syslog(LOG_ERR, "cFont::LoadFT2: glcdgraphics was compiled without FreeType2 support!!!");
    return false;
#endif
}

int cFont::Width(int ch) const
{
    const cBitmap *bitmap = GetCharacter(ch);
    if (bitmap)
        return bitmap->Width();
    else
        return 0;
}

int cFont::Width(const std::string & str) const
{
    unsigned int i;
    int sum = 0;
    int c,c0,c1,c2,c3,symcount=0;

    for (i = 0; i < str.length(); i++)
    {
        c = str[i];
        c0 = str[i];
        c1 = (i+1 < str.length()) ? str[i+1] : 0;
        c2 = (i+2 < str.length()) ? str[i+2] : 0;
        c3 = (i+3 < str.length()) ? str[i+3] : 0;
        c0 &=0xff; c1 &=0xff; c2 &=0xff; c3 &=0xff;

        if( c0 >= 0xc2 && c0 <= 0xdf && c1 >= 0x80 && c1 <= 0xbf ){ //2 byte UTF-8 sequence found
            i+=1;
            c = ((c0&0x1f)<<6) | (c1&0x3f);
        }else if(  (c0 == 0xE0 && c1 >= 0xA0 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) 
                || (c0 >= 0xE1 && c1 <= 0xEC && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) 
                || (c0 == 0xED && c1 >= 0x80 && c1 <= 0x9f && c2 >= 0x80 && c2 <= 0xbf) 
                || (c0 >= 0xEE && c0 <= 0xEF && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) ){  //3 byte UTF-8 sequence found
            c = ((c0&0x0f)<<4) | ((c1&0x3f)<<6) | (c2&0x3f);
            i+=2;
        }else if(  (c0 == 0xF0 && c1 >= 0x90 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) 
                || (c0 >= 0xF1 && c0 >= 0xF3 && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) 
                || (c0 == 0xF4 && c1 >= 0x80 && c1 <= 0x8f && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) ){  //4 byte UTF-8 sequence found
            c = ((c0&0x07)<<2) | ((c1&0x3f)<<4) | ((c2&0x3f)<<6) | (c3&0x3f);
            i+=3;
        }
        symcount++;
        sum += Width(c);
    }
    sum += spaceBetween * (symcount - 1);
    return sum;
}

int cFont::Width(const std::string & str, unsigned int len) const
{
    unsigned int i;
    int sum = 0;

    int c,c0,c1,c2,c3,symcount=0; 

    for (i = 0; i < str.length() && symcount < len; i++)
    {
        c = str[i];
        c0 = str[i];
        c1 = (i+1 < str.length()) ? str[i+1] : 0;
        c2 = (i+2 < str.length()) ? str[i+2] : 0;
        c3 = (i+3 < str.length()) ? str[i+3] : 0;
        c0 &=0xff; c1 &=0xff; c2 &=0xff; c3 &=0xff;

        if( c0 >= 0xc2 && c0 <= 0xdf && c1 >= 0x80 && c1 <= 0xbf ){ //2 byte UTF-8 sequence found
            i+=1;
            c = ((c0&0x1f)<<6) | (c1&0x3f);
        }else if(  (c0 == 0xE0 && c1 >= 0xA0 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) 
                || (c0 >= 0xE1 && c1 <= 0xEC && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) 
                || (c0 == 0xED && c1 >= 0x80 && c1 <= 0x9f && c2 >= 0x80 && c2 <= 0xbf) 
                || (c0 >= 0xEE && c0 <= 0xEF && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf) ){  //3 byte UTF-8 sequence found
            c = ((c0&0x0f)<<4) | ((c1&0x3f)<<6) | (c2&0x3f);
            i+=2;
        }else if(  (c0 == 0xF0 && c1 >= 0x90 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) 
                || (c0 >= 0xF1 && c0 >= 0xF3 && c1 >= 0x80 && c1 <= 0xbf && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) 
                || (c0 == 0xF4 && c1 >= 0x80 && c1 <= 0x8f && c2 >= 0x80 && c2 <= 0xbf && c3 >= 0x80 && c3 <= 0xbf) ){  //4 byte UTF-8 sequence found
            c = ((c0&0x07)<<2) | ((c1&0x3f)<<4) | ((c2&0x3f)<<6) | (c3&0x3f);
            i+=3;
        }
        symcount++;
        sum += Width(c);
    }
    sum += spaceBetween * (symcount - 1);

    return sum;
}

int cFont::Height(int ch) const
{
    const cBitmap *bitmap = GetCharacter(ch);
    if (bitmap)
        return bitmap->Height();
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

const cBitmap * cFont::GetCharacter(int ch) const
{
#ifdef HAVE_FREETYPE2
    if ( loadedFontType == lftFT2 ) {
        //lookup in cache
        cBitmap *ptr=characters_cache->GetBitmap(ch);
        if (ptr)
            return ptr;

        FT_Face face = (FT_Face) ft2_face;
        FT_UInt glyph_index;
        //Get FT char index
        glyph_index = FT_Get_Char_Index(face, ch);

        //Load the char
        int error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (error)
        {
            syslog(LOG_ERR, "cFont::LoadFT2: ERROR when calling FT_Load_Glyph: %x", error);
            return NULL;
        }

        FT_Render_Mode  rmode = FT_RENDER_MODE_MONO;
#if ( (FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 7) || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 2 && FREETYPE_PATCH <= 1) )
        if (ch == 32) rmode = FT_RENDER_MODE_NORMAL;
#endif

        // convert to a mono bitmap
        error = FT_Render_Glyph(face->glyph, rmode);
        if (error)
        {
            syslog(LOG_ERR, "cFont::LoadFT2: ERROR when calling FT_Render_Glyph: %x", error);
            return NULL;
        } else {
            // now, fill our pixel data
            cBitmap *charBitmap = new cBitmap(face->glyph->advance.x >> 6, TotalHeight());
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

            // adjust maxwidth if necessary
            //if (totalWidth < charBitmap->Width())
            //    totalWidth = charBitmap->Width();

            characters_cache->PushBack(ch, charBitmap);
            return charBitmap;
        }
        return NULL; // if any
    } // else
#endif
    return characters[(unsigned char) ch];
}

void cFont::SetCharacter(char ch, cBitmap * bitmapChar)
{
#ifdef HAVE_FREETYPE2
    if ( loadedFontType == lftFT2 ) {
        syslog(LOG_ERR, "cFont::SetCharacter: is not supported with FreeType2 fonts!!!");
        return;
    }
#endif
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
    loadedFontType = lftFNT;
#ifdef HAVE_FREETYPE2
    ft2_library = NULL;
    ft2_face = NULL;
    characters_cache = NULL;
#endif
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
#ifdef HAVE_FREETYPE2
    delete characters_cache;
    if (ft2_face)
        FT_Done_Face((FT_Face)ft2_face);
    if (ft2_library)
        FT_Done_FreeType((FT_Library)ft2_library);
#endif
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
