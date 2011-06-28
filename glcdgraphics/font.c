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
 * (c) 2004-2010 Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2010-2011 Wolfgang Astleitner <mrwastl AT users sourceforge net>
 *               Andreas 'randy' Weinberger
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

#ifdef HAVE_FREETYPE2

class cBitmapCache
{
private:
protected:
    cBitmapCache *next;  // next  bitmap
    cBitmap      *ptr;
    uint32_t     charcode;
public:
    cBitmapCache();
    ~cBitmapCache();

    void PushBack(uint32_t ch, cBitmap *bitmap);
    cBitmap *GetBitmap(uint32_t ch) const;
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

void cBitmapCache::PushBack(uint32_t ch, cBitmap *bitmap)
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

cBitmap *cBitmapCache::GetBitmap(uint32_t ch) const
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

bool cFont::LoadFNT(const std::string & fileName, const std::string & encoding)
{
    // cleanup if we already had a loaded font
    Unload();
    fontType = 1; //original fonts
    isutf8 = (encoding == "UTF-8");

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
        syslog(LOG_ERR, "cFont::LoadFNT(): Cannot open file: %s - not the correct fileheader.\n",fileName.c_str());
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
#ifdef DEBUG
        printf ("fontHeight %0d - charWidth %0d - character %0d - bytes %0d\n", fontHeight, charWidth, character,fontHeight * ((charWidth + 7) / 8));
#endif
 
        int y; int loop; 
        int num = 0;
        uint dot; uint b;
        cBitmap * charBitmap = new cBitmap(charWidth, fontHeight);
        charBitmap->SetMonochrome(true);
        charBitmap->Clear();
        for (num=0; num<fontHeight * ((charWidth + 7) / 8);num++) {
            y = (charWidth + 7) / 8;
            for (loop=0;loop<((charWidth + 7) / 8); loop++) {
                for (b=0;b<charWidth;b++) {
                    dot=buffer[num+loop] & (0x80 >> b);
                    if (dot) {
                        charBitmap->DrawPixel(b+((loop*8)),num/y,cColor::Black);
                    }
                }
            }
            num=num+y-1;
        }
        SetCharacter(character, charBitmap);

        if (charWidth > maxWidth)
            maxWidth = charWidth;
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
        if (GetCharacter(i))
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

    const cBitmap* charbmp = NULL;
    for (i = 0; i < 256; i++)
    {
        charbmp = GetCharacter(i);
        if (charbmp)
        {
            chdr[0] = (uint8_t) i;
            chdr[1] = (uint8_t) (i >> 8);
            chdr[2] = (uint8_t) charbmp->Width();
            chdr[3] = (uint8_t) (charbmp->Width() >> 8);
            fwrite(chdr, kCharHeaderSize, 1, fontFile);
            const unsigned char* monobmp = cBitmap::ConvertTo1BPP(*charbmp);
            fwrite(monobmp /*characters[i]->Data()*/, totalHeight * ((charbmp->Width() + 7) / 8), 1, fontFile);
            delete[] monobmp;
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
    fontType = 2; // ft2 fonts
    isutf8 = (encoding == "UTF-8");

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

    // generate lookup table for encoding conversions (encoding != UTF8)
    if (! (isutf8  || dingBats) )
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
            iconv_lut[c] = ((size_t) -1 == count) ? (wchar_t)'?' : wchar_buff;
        }
        iconv_close(cd);
    } else {
        // don't leave LUT uninitialised
        for (int c = 0; c < 256; c++)
            iconv_lut[c] = (wchar_t)c;
    }

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

int cFont::Width(uint32_t ch) const
{
    const cBitmap *bitmap = GetCharacter(ch);
    if (bitmap)
        return bitmap->Width();
    else
        return 0;
}

int cFont::Width(const std::string & str) const
{
    return Width(str, (unsigned int) str.length());
}

int cFont::Width(const std::string & str, unsigned int len) const
{
    unsigned int i;
    int sum = 0;
    unsigned int symcount=0;
    uint32_t c;

    i = 0;
    while (i < (unsigned int)str.length() && symcount < len)
    {
        encodedCharAdjustCounter(IsUTF8(), str, c, i);
        symcount++;
        sum += Width(c);
        i++;
    }
    sum += spaceBetween * (symcount - 1);

    return sum;
}

int cFont::Height(uint32_t ch) const
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

const cBitmap * cFont::GetCharacter(uint32_t ch) const
{
#ifdef HAVE_FREETYPE2
    if ( fontType == 2 ) {
        //lookup in cache
        cBitmap *ptr=characters_cache->GetBitmap(ch);
        if (ptr)
            return ptr;

        FT_Face face = (FT_Face) ft2_face;
        FT_UInt glyph_index;
        //Get FT char index
        if (isutf8) {
            glyph_index = FT_Get_Char_Index(face, ch);
        } else {
            glyph_index = FT_Get_Char_Index(face, iconv_lut[(unsigned char)ch]);
        }

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
            charBitmap->Clear(cColor::White);
            charBitmap->SetMonochrome(true);
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
                                              /*GLCD::clrBlack*/ cColor::Black);
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
    if ( fontType == 2 ) {
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
#ifdef HAVE_FREETYPE2
    ft2_library = NULL;
    ft2_face = NULL;
    characters_cache = NULL;
#endif
    fontType = 1;
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
    unsigned int pos;
    std::string::size_type posLast;
    uint32_t c;

    Lines.clear();
    maxLines = 100;
    if (Height > 0)
    {
        maxLines = Height / LineHeight();
        //if (maxLines == 0)
            maxLines += 1;
    }

    lineCount = 0;
    pos = 0;
    start = 0;
    posLast = 0;
    textWidth = 0;

    while ((pos < Text.length()) && (lineCount <= maxLines))
    {
        unsigned int posraw = pos;
        encodedCharAdjustCounter(IsUTF8(), Text, c, posraw);
        
        if (c == '\n')
        {
            Lines.push_back(trim(Text.substr(start, pos - start)));
            start = pos /*+ 1*/;
            posLast = pos /*+ 1*/;
            textWidth = 0;
            lineCount++;
        }
        else if (textWidth + this->Width(c) > Width  && (lineCount + 1) < maxLines)
        {
            if (posLast > start)
            {
                Lines.push_back(trim(Text.substr(start, posLast - start)));
                start = posLast /*+ 1*/;
                posLast = start;
                textWidth = this->Width(Text.substr(start, pos - start + 1)) + spaceBetween;
            }
            else
            {
                Lines.push_back(trim(Text.substr(start, pos - start)));
                start = pos /*+ 1*/;
                posLast = start;
                textWidth = this->Width(c) + spaceBetween;
            }
            lineCount++;
        }
        else if (isspace(c))
        {
            posLast = pos;
            textWidth += this->Width(c) + spaceBetween;
        }
        else if ( (c < 0x80) && strchr("-.,:;!?_", (int)c) )
        {
            posLast = pos+1;
            textWidth += this->Width(c) + spaceBetween;
        }
        else
        {
            textWidth += this->Width(c) + spaceBetween;
        }
        pos = posraw;
        pos++;
    }
    if (start < Text.length()) {
        Lines.push_back(trim(Text.substr(start)));
    }

    if (ActualWidth) {
        textWidth = 0;
        for (int i = 0; i < lineCount; i++)
            textWidth = std::max(textWidth, this->Width(Lines[i]));
        textWidth = std::min(textWidth, Width);
        *ActualWidth = textWidth;
    }
}

} // end of namespace
