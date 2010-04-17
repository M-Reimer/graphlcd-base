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

#include "xml.h"

namespace GLCD
{

std::string trim(const std::string & s)
{
    std::string::size_type start, end;

    start = 0;
    while (start < s.length())
    {
        if (!isspace(s[start]))
            break;
        start++;
    }
    end = s.length() - 1;
    while (end >= 0)
    {
        if (!isspace(s[end]))
            break;
        end--;
    }
    return s.substr(start, end - start + 1);
}

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

cXML::cXML(const std::string & file)
:   nodestartcb(NULL),
    nodeendcb(NULL),
    cdatacb(NULL),
    parseerrorcb(NULL),
    progresscb(NULL)
{
    char * buffer;
    long size;

#if (__GNUC__ < 3)
    std::ifstream f(file.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
#else
    std::ifstream f(file.c_str(), std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
#endif
    if (!f.is_open())
    {
        syslog(LOG_ERR, "ERROR: skin file %s not found\n", file.c_str());
    }
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

cXML::cXML(const char * mem, unsigned int len)
:   nodestartcb(NULL),
    nodeendcb(NULL),
    cdatacb(NULL),
    parseerrorcb(NULL),
    progresscb(NULL)
{
    data.assign(mem, len);
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

    state    = LOOK4START;
    linenr   = 1;
    skipping = false;
    len = data.length();
    for (std::string::size_type i = 0; i < len; i++)
    {
        if (ReadChar(data[i]) != 0)
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
    }
    return 0;
}

bool cXML::IsTokenChar(bool start, int c)
{
    return isalpha(c) || c == '_' || (!start && isdigit(c));
}

int cXML::ReadChar(int c)
{
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
                cdata += c;
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
