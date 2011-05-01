/**
 *  GraphLCD plugin for the Video Disk Recorder
 *
 *  tuxbox.h  -  tuxbox logo class
 *
 *  (c) 2004 Andreas Brachold <vdr04 AT deltab de>
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
#ifndef _TUXBOX_H_
#define _TUXBOX_H_

#include <glcdgraphics/imagefile.h>

class cTuxBoxFile : public GLCD::cImageFile
{
private:
//    void vert2horz(const unsigned char* source, unsigned char* dest, int width, int height);
//    void horz2vert(const unsigned char* source, unsigned char* dest, int width, int height);
    void vert2horz(const uint32_t *source, uint32_t *dest, int width, int height);
    void horz2vert(const uint32_t *source, uint32_t *dest, int width, int height);
public:
    cTuxBoxFile();
    virtual ~cTuxBoxFile();
    virtual bool Load(GLCD::cImage & image, const std::string & fileName);
    virtual bool Save(GLCD::cImage & image, const std::string & fileName);
};

#endif
