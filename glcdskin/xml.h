/*
 * GraphLCD skin library
 *
 * xml.h  -  xml parser class
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * based on text2skin xml parser
 *
 */

#ifndef _GLCDSKIN_XML_H_
#define _GLCDSKIN_XML_H_

#include <string>
#include <map>
#include <iconv.h>

namespace GLCD
{

#define XML_NODE_START_CB(CB) \
bool (*CB)(const std::string &tag, std::map<std::string, std::string> &attr)
#define XML_NODE_END_CB(CB) \
bool (*CB)(const std::string &tag)
#define XML_CDATA_CB(CB) \
bool (*CB)(const std::string &text)
#define XML_PARSE_ERROR_CB(CB) \
void (*CB)(int line, const char *txt, char c)
#define XML_PROGRESS_CB(CB) \
void (*CB)(int percent)

class cXML
{
private:
    bool validFile;
    bool skipping;
    int state;
    int linenr;
    unsigned int delim;
    std::string sysEncoding;
    bool sysIsUTF8;
    iconv_t iconv_cd;

    std::string data, cdata, tag, attrn, attrv;
    std::map<std::string, std::string> attr;

    XML_NODE_START_CB(nodestartcb);
    XML_NODE_END_CB(nodeendcb);
    XML_CDATA_CB(cdatacb);
    XML_PARSE_ERROR_CB(parseerrorcb);
    XML_PROGRESS_CB(progresscb);

protected:
    bool IsTokenChar(bool start, int c);
    int  ReadChar(unsigned int c, int char_size);

public:
    cXML(const std::string & file, const std::string sysCharset = "UTF-8");
    //cXML(const char * mem, unsigned int len);
    ~cXML();

    void SetNodeStartCB(XML_NODE_START_CB(cb));
    void SetNodeEndCB(XML_NODE_END_CB(cb));
    void SetCDataCB(XML_CDATA_CB(cb));
    void SetParseErrorCB(XML_PARSE_ERROR_CB(cb));
    void SetProgressCB(XML_PROGRESS_CB(cb));

    int Parse(void);

    int LineNr(void) const { return linenr; }
};

} // end of namespace

#endif
