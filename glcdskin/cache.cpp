/*
 * GraphLCD skin library
 *
 * cache.c  -  image cache
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 */

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/image.h>
#include <glcdgraphics/glcd.h>
#include <glcdgraphics/pbm.h>
#include <glcdgraphics/extformats.h>

#include <stdlib.h>
#include <string.h>

#include <syslog.h>

#include "cache.h"
#include "skin.h"

namespace GLCD
{

cImageItem::cImageItem(const std::string & path, cImage * image, uint16_t scalew, uint16_t scaleh)
:   path(path),
    counter(0),
    image(image),
    scale_width(scalew),
    scale_height(scaleh)
{
}

cImageItem::~cImageItem()
{
    delete image;
}


cImageCache::cImageCache(cSkin * Parent, int Size)
:   skin(Parent),
    size(Size)
{
}

cImageCache::~cImageCache()
{
    Clear();
}

void cImageCache::Clear(void)
{
    for (unsigned int i = 0; i < images.size(); i++)
    {
        delete images[i];
    }
    images.clear();
    failedpaths.clear();
}

cImage * cImageCache::Get(const std::string & path, uint16_t & scalew, uint16_t & scaleh)
{
    std::vector <cImageItem *>::iterator it;
    cImageItem * item;
    uint64_t maxCounter;
    std::vector <cImageItem *>::iterator oldest;

    // test if this path has already been stored as invalid path / invalid/non-existent image
    for (size_t i = 0; i < failedpaths.size(); i++) {
      if (failedpaths[i] == path) {
        return NULL;
      }
    }

    maxCounter = 0;
    item = NULL;
    for (it = images.begin(); it != images.end(); it++)
    {
        uint16_t scw = 0, sch = 0;
        (*it)->ScalingGeometry(scw, sch);
        if (item == NULL && path == (*it)->Path() && ( (scw == 0 && sch == 0 && ! (scalew || scaleh)) || (scw == scalew && sch == scaleh)))
        {
            (*it)->ResetCounter();
            item = (*it);
        }
        else
        {
            (*it)->IncCounter();
            if ((*it)->Counter() > maxCounter)
            {
                maxCounter = (*it)->Counter();
                oldest = it;
            }
        }
    }
    if (item)
    {
        return item->Image();
    }

    item = LoadImage(path, scalew, scaleh);
    if (item)
    {
        syslog(LOG_INFO, "INFO: graphlcd: successfully loaded image '%s'\n", path.c_str());
        if (images.size() == size)
        {
            images.erase(oldest);
        }
        images.push_back(item);
        return item->Image();
    } else {
      failedpaths.push_back(path);
    }
    return NULL;
}

cImageItem * cImageCache::LoadImage(const std::string & path, uint16_t scalew, uint16_t scaleh)
{
    //fprintf(stderr, "### loading image  %s\n", path.c_str());
    cImageItem * item;
    cImage * image;
    char str[8];
    int i;
    int j;
    std::string file;

    i = path.length() - 1;
    j = 0;
    while (i >= 0 && j < 6)
    {
        if (path[i] == '.')
            break;
        i--;
        j++;
    }
    i++;
    j = 0;
    while (i < (int) path.length())
    {
        str[j] = toupper((unsigned) path[i]);
        i++;
        j++;
    }
    str[j] = 0;

    image = new cImage();
    if (!image)
        return NULL;

    if (path[0] == '/' || path.find("./") == 0 || path.find("../") == 0)
        file = path;
    else
    {
        file = skin->Config().SkinPath();
        if (file.length() > 0)
        {
            if (file[file.length() - 1] != '/')
                file += '/';
        }
        file += path;
    }
    
    cImageFile* imgFile = NULL;

    if (strcmp(str, "PBM") == 0) {
        imgFile = new cPBMFile();
    } else if (strcmp(str, "GLCD") == 0) {
        imgFile = new cGLCDFile();
    } else {
        imgFile = new cExtFormatFile();
    }

    uint16_t scale_width = scalew;
    uint16_t scale_height = scaleh;
    // scale_width and scale_height are set to 0 if image was NOT scaled
    if (!imgFile || (imgFile->LoadScaled(*image, file, scalew /*scale_width*/, scaleh /*scale_height*/) == false) ) {
        delete image;
        if (imgFile) delete imgFile;
        return NULL;
    }
    delete imgFile;
    
#if 0
    if (strcmp(str, "PBM") == 0)
    {
        cPBMFile pbm;

        if (pbm.Load(*image, file) == false)
        {
            delete image;
            return NULL;
        }
    }
    else if (strcmp(str, "GLCD") == 0)
    {
        cGLCDFile glcd;

        if (glcd.Load(*image, file) == false)
        {
            delete image;
            return NULL;
        }
    }
    else
    {
        cExtFormatFile extformat;

        if (extformat.Load(*image, file) == false)
        {
          delete image;
          return NULL;
        }
    }
#endif

    item = new cImageItem(path, image, scale_width, scale_height);
    if (!item)
    {
        delete image;
        return NULL;
    }
    return item;
}

} // end of namespace

