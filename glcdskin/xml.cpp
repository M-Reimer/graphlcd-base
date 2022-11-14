/*
 * GraphLCD skin library
 *
 * xml.c  -  xml parser class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * This module was kindly provided by Clemens Kirchgatterer
 */

#include <ctype.h>
#include <syslog.h>

#include <iostream>
#include <fstream>

#include <string.h>
#include <stdlib.h>

#include "xml.h"
#include "../glcdgraphics/common.h"

namespace GLCD
{

enum {
    LOOK4START,     // looking for first element start
    LOOK4TAG,       // looking for element tag
    INTAG,          // reading tag
    INCOMMENT,      // reading comment
    LOOK4CEND1,     // looking for second '-' in -->
    LOOK4CEND2,     // looking for '>' in -->
    LOOK4ATTRN,     // looking for attr name, > or /
    INATTRN,        // reading attr name
    LOOK4ATTRV,     // looking for attr value
    SAWSLASH,       // saw / in element opening
    INATTRV,        // in attr value
    LOOK4CLOSETAG,  // looking for closing tag after <
    INCLOSETAG,     // reading closing tag
};

cXML::cXML(const std::string & file, const std::string sysCharset)
:   nodestartcb(NULL),
    nodeendcb(NULL),
    cdatacb(NULL),
    parseerrorcb(NULL),
    progresscb(NULL)
{
    char * buffer;
    long size;
    sysEncoding = sysCharset;
    sysIsUTF8 = (sysEncoding == "UTF-8");
    if (!sysIsUTF8) {
        // convert from utf-8 to system encoding
        iconv_cd = iconv_open(sysEncoding.c_str(), "UTF-8");
        if (iconv_cd == (iconv_t) -1) {
            syslog(LOG_ERR, "ERROR: system encoding %s is not supported\n", sysEncoding.c_str());
            iconv_cd = NULL;
        }
    } else {
        iconv_cd = NULL;
    }

#if (__GNUC__ < 3)
    std::ifstream f(file.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
#else
    std::ifstream f(file.c_str(), std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
#endif
    if (!f.is_open())
    {
        syslog(LOG_ERR, "ERROR: skin file %s not found\n", file.c_str());
        validFile = false;
    } else {
        validFile = true;
        size = f.tellg();
#if (__GNUC__ < 3)
        f.seekg(0, std::ios::beg);
#else
        f.seekg(0, std::ios_base::beg);
#endif
        buffer = new char[size];
        f.read(buffer, size);
        f.close();
        data.assign(buffer, size);
        delete[] buffer;
    }
}

#if 0
cXML::cXML(const char * mem, unsigned int len)
:   nodestartcb(NULL),
    nodeendcb(NULL),
    cdatacb(NULL),
    parseerrorcb(NULL),
    progresscb(NULL)
{
    data.assign(mem, len);
}
#endif

cXML::~cXML()
{
    if (iconv_cd != NULL)
        iconv_close(iconv_cd);
}

void cXML::SetNodeStartCB(XML_NODE_START_CB(cb))
{
    nodestartcb = cb;
}

void cXML::SetNodeEndCB(XML_NODE_END_CB(cb))
{
    nodeendcb = cb;
}

void cXML::SetCDataCB(XML_CDATA_CB(cb))
{
    cdatacb = cb;
}

void cXML::SetParseErrorCB(XML_PARSE_ERROR_CB(cb))
{
    parseerrorcb = cb;
}

void cXML::SetProgressCB(XML_PROGRESS_CB(cb))
{
    progresscb = cb;
}

int cXML::Parse(void)
{
    int percent = 0;
    int last = 0;
    std::string::size_type len;
    uint32_t c, c_tmp;
    unsigned int i_old;
    int l, char_size;
    
    if (!validFile)
        return -1;

    state    = LOOK4START;
    linenr   = 1;
    skipping = false;
    len = data.length();

    unsigned int i = 0;    
    while (i < (unsigned int)len)
    {
        i_old = i;
        encodedCharAdjustCounter(true, data, c_tmp, i);
        char_size = (i - i_old) + 1;
        c = 0;
        for (l = 0 ; l < char_size; l++)
            c += ( (0xFF & data[i_old + l]) << ( l << 3) );

        if (ReadChar(c /*data[i]*/, char_size) != 0)
            return -1;
        if (progresscb)
        {
            percent = i * 100 / len;
            if (percent > last)
            {
                progresscb(percent);
                last = percent;
            }
        }
        i++;
    }
    return 0;
}

bool cXML::IsTokenChar(bool start, int c)
{
    return isalpha(c) || c == '_' || (!start && isdigit(c));
}

int cXML::ReadChar(unsigned int c, int char_size)
{
    // buffer for conversions (when conversion from utf8 to system encoding is required)
    char convbufin[5];
    char convbufout[5];
    char* convbufinp = convbufin;
    char* convbufoutp = convbufout;
    size_t bufin_size, bufout_size, bufconverted;

    // new line?
    if (c == '\n')
        linenr++;

    switch (state)
    {
            // looking for element start
        case LOOK4START:
            if (c == '<')
            {
                if (cdatacb)
                {
                    std::string::size_type pos = 0;
                    while ((pos = cdata.find('&', pos)) != std::string::npos)
                    {
                        if (cdata.substr(pos, 4) == "&lt;")
                            cdata.replace(pos, 4, "<");
                        else if (cdata.substr(pos, 4) == "&gt;")
                            cdata.replace(pos, 4, ">");
                        else if (cdata.substr(pos, 5) == "&amp;")
                            cdata.replace(pos, 5, "&");
                        else if (cdata.substr(pos, 2) == "&#") {
                            bool ishex = ((cdata.substr(pos+2, 1) == "x") || (cdata.substr(pos+2, 1) == "X") );
                            size_t startpos = pos+2+((ishex)?1:0);
                            size_t endpos = cdata.find(';', startpos );
                            if (endpos != std::string::npos) {
                                char* tempptr;
                                std::string charid = cdata.substr(startpos, endpos-startpos);
                                long val = strtol(charid.c_str(), &tempptr, (ishex) ? 16 : 10);

                                if (tempptr != charid.c_str() && *tempptr == '\0') {
                                    char encbuf[5]; size_t enclen = 0;

                                    if ( val <= 0x1F ) {
                                        enclen = 0;  // ignore control chars
                                    } else if ( val <= 0x007F ) {
                                        enclen = 1;
                                        encbuf[0] = (char)(val & 0x7F);
                                    } else if ( val <= 0x07FF ) {
                                        enclen = 2;
                                        encbuf[1] = (char)(( val & 0x003F) | 0x80);
                                        encbuf[0] = (char)(( (val & 0x07C0) >> 6) | 0xC0);
                                    } else if ( val <= 0xFFFF ) {
                                        enclen = 3;
                                        encbuf[2] = (char)(( val & 0x003F) | 0x80);
                                        encbuf[1] = (char)(( (val & 0x0FC0) >> 6) | 0x80);
                                        encbuf[0] = (char)(( (val & 0xF000) >> 12) | 0xE0);
                                    } else if ( val <= 0x10FFFF ) {
                                        enclen = 4;
                                        encbuf[3] = (char)(( val & 0x003F) | 0x80);
                                        encbuf[2] = (char)(( (val & 0x0FC0) >> 6) | 0x80);
                                        encbuf[1] = (char)(( (val & 0x03F000 ) >> 12) | 0x80);
                                        encbuf[0] = (char)(( (val & 0x1C0000 ) >> 18) | 0xF0);
                                    }
                                    encbuf[enclen] = '\0';
                                    if (enclen > 0) {
                                        cdata.replace(pos, endpos-pos+1, encbuf);
                                    }
                                }
                            }
                        }
                        pos++;
                    }
                    if (!cdatacb(trim(cdata)))
                        return -1;
                }
                cdata = "";
                attr.clear();
                tag = "";
                state = LOOK4TAG;
            }
            else
            {
                int i;
                //cdata += c;
                // convert text-data on the fly if system encoding != UTF-8
                if (iconv_cd != NULL && char_size > 1 /* ((c & 0x80) == 0x80)*/) {
                    for (i = 0; i < char_size; i++)
                        convbufin[i] = ( (char)((c  >> ( i << 3) ) & 0xFF) );
                    convbufin[char_size] = '\0';
                    bufin_size = strlen(convbufin);
                    bufout_size = bufin_size;
                    bufconverted = iconv(iconv_cd, &convbufinp, &bufin_size, &convbufoutp, &bufout_size);

                    if (bufconverted != (size_t)-1 && strlen(convbufout) != 0) {
                        for (i = 0; i < (int)strlen(convbufout); i++)
                            cdata += convbufout[i];
                    } else {
                        cdata += "?";
                    }
                } else {
                    for (i = 0; i < char_size; i++)
                        cdata += ( (unsigned char)((c  >> ( i << 3) ) & 0xFF) );
                }
            }
            // silently ignore until resync
            break;

            // looking for element tag
        case LOOK4TAG:
            // skip comments and declarations.
            if (skipping)
            {
                if (c == '>')
                {
                    skipping = false;
                    state = LOOK4START;
                }
                break;
            }
            else
            {
                if (c == '?')
                {
                    skipping = true;
                    break;
                }
            }
            if (IsTokenChar(true, c))
            {
                tag += c;
                state = INTAG;
            }
            else if (c == '/')
            {
                state = LOOK4CLOSETAG;
            }
            else if (c == '!')
            {
                state = INCOMMENT;
            }
            else if (!isspace(c))
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "Bogus tag char", c);
                }
                return -1;
            }
            break;

            // reading tag
        case INTAG:
            if (IsTokenChar(false, c))
            {
                tag += c;
            }
            else if (c == '>')
            {
                if (nodestartcb)
                    if (!nodestartcb(tag, attr))
                        return -1;
                state = LOOK4START;
            }
            else if (c == '/')
            {
                state = SAWSLASH;
            }
            else
            {
                state = LOOK4ATTRN;
            }
            break;

            // reading comment
        case INCOMMENT:
            if (c == '-')
            {
                state = LOOK4CEND1;
            }
            break;

            // looking for second '-' in "-->"
        case LOOK4CEND1:
            if (c == '-')
            {
                state = LOOK4CEND2;
            }
            else
            {
                state = INCOMMENT;
            }
            break;

            // looking for '>' in "-->"
        case LOOK4CEND2:
            if (c == '>')
            {
                state = LOOK4START;
            }
            else if (c != '-')
            {
                state = INCOMMENT;
            }
            break;

            // looking for attr name, > or /
        case LOOK4ATTRN:
            if (c == '>')
            {
                if (nodestartcb)
                    if (!nodestartcb(tag, attr))
                        return -1;
                state = LOOK4START;
            }
            else if (c == '/')
            {
                state = SAWSLASH;
            }
            else if (IsTokenChar(true, c))
            {
                attrn = "";
                attrn += c;
                state = INATTRN;
            }
            else if (!isspace(c))
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "Bogus 1st attr name char", c);
                }
                return -2;
            }
            break;

            // saw / in element opening
        case SAWSLASH:
            if (c == '>')
            {
                if (nodestartcb)
                    if (!nodestartcb(tag, attr))
                        return -1;
                if (nodeendcb)
                    if (!nodeendcb(tag))
                        return -1;
                state = LOOK4START;
            }
            else
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "Bogus char before >", c);
                }
                return -3;
            }
            break;

            // reading attr name
        case INATTRN:
            if (IsTokenChar(false, c))
            {
                attrn += c;
            }
            else if (isspace(c) || c == '=')
            {
                state = LOOK4ATTRV;
            }
            else
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "Bogus attr name char", c);
                }
                return -4;
            }
            break;

            // looking for attr value
        case LOOK4ATTRV:
            if (c == '\'' || c == '"')
            {
                delim = c;
                attrv = "";
                state = INATTRV;
            }
            else if (!(isspace(c) || c == '='))
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "No attribute value", c);
                }
                return -5;
            }
            break;

            // in attr value
        case INATTRV:
            if (c == delim)
            {
                attr[attrn] = attrv;
                state = LOOK4ATTRN;
            }
            else if (!iscntrl(c))
            {
                attrv += c;
            }
            break;

            // looking for closing tag after <
        case LOOK4CLOSETAG:
            if (IsTokenChar(true, c))
            {
                tag += c;
                state = INCLOSETAG;
            }
            else if (!isspace(c))
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "Bogus preend tag char", c);
                }
                return -6;
            }
            break;

            // reading closing tag
        case INCLOSETAG:
            if (IsTokenChar(false, c))
            {
                tag += c;
            }
            else if (c == '>')
            {
                if (nodeendcb)
                    if (!nodeendcb(tag))
                        return 0;//XXX is that right? there was false before
                state = LOOK4START;
            }
            else if (!isspace(c))
            {
                if (parseerrorcb)
                {
                    parseerrorcb(linenr, "Bogus end tag char", c);
                }
                return -7;
            }
            break;
    }

    return 0;
}

} // end of namespace
