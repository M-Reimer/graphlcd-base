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
public:
    cImageItem(const std::string & path, cImage * image);
    ~cImageItem();

    const std::string & Path() const { return path; }
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

    cImageItem * LoadImage(const std::string & path);
public:
    cImageCache(cSkin * Parent, int Size);
    ~cImageCache();

    cImage * Get(const std::string & path);
};

} // end of namespace

#endif
