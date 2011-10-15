/*
 * GraphLCD skin library
 *
 * cache.h  -  image cache
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 */

#ifndef _GLCDSKIN_CACHE_H_
#define _GLCDSKIN_CACHE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include <glcdgraphics/image.h>

namespace GLCD
{

class cSkin;

class cImageItem
{
private:
    std::string path;
    uint64_t counter;
    cImage * image;
    uint16_t scale_width, scale_height;
public:
    cImageItem(const std::string & path, cImage * image, uint16_t scalew, uint16_t scaleh);
    ~cImageItem();

    const std::string & Path() const { return path; }
    void ScalingGeometry(uint16_t & scalew, uint16_t & scaleh) { scalew = scale_width; scaleh = scale_height; }
    uint64_t Counter() const { return counter; }
    cImage * Image() { return image; }
    void ResetCounter() { counter = 0; }
    void IncCounter() { counter += 1; }
};

class cImageCache
{
private:
    cSkin * skin;
    size_t size;
    std::vector <cImageItem *> images;
    std::vector <std::string> failedpaths;

    cImageItem * LoadImage(const std::string & path, uint16_t scalew, uint16_t scaleh);
public:
    cImageCache(cSkin * Parent, int Size);
    ~cImageCache();

    cImage * Get(const std::string & path, uint16_t & scalew, uint16_t & scaleh);
    cImage * Get(const std::string & path) {
        uint16_t scalew = 0;
        uint16_t scaleh = 0;
        return Get(path, scalew, scaleh) ;
    }
    
    void Clear(void);
};

} // end of namespace

#endif
