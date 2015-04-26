#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include <string>

#include <glcdgraphics/bitmap.h>
#include <glcdgraphics/font.h>
#include <glcddrivers/config.h>
#include <glcddrivers/driver.h>
#include <glcddrivers/drivers.h>
#include <glcdskin/parser.h>
#include <glcdskin/skin.h>
#include <glcdskin/config.h>
#include <glcdskin/string.h>

class cMySkinConfig : public GLCD::cSkinConfig
{
private:
    GLCD::cDriver * mDriver;
public:
    cMySkinConfig(GLCD::cDriver * Driver);
    virtual std::string SkinPath(void);
    virtual std::string CharSet(void);
    virtual std::string Translate(const std::string & Text);
    virtual GLCD::cType GetToken(const GLCD::tSkinToken & Token);
    virtual GLCD::cDriver * GetDriver(void) const { return mDriver; }
};

cMySkinConfig::cMySkinConfig(GLCD::cDriver * Driver)
:   mDriver(Driver)
{
}

std::string cMySkinConfig::SkinPath(void)
{
    return ".";
}

std::string cMySkinConfig::CharSet(void)
{
    return "iso8859-15";
}

std::string cMySkinConfig::Translate(const std::string & Text)
{
    return Text;
}

GLCD::cType cMySkinConfig::GetToken(const GLCD::tSkinToken & Token)
{
    return 10;
}


static const char * kDefaultConfigFile = "/etc/graphlcd.conf";

int main(int argc, char ** argv)
{
    static struct option long_options[] =
    {
        {"config",     required_argument, NULL, 'c'},
        {"display",    required_argument, NULL, 'd'},
        {"skin",       required_argument, NULL, 's'},
        {"upsidedown",       no_argument, NULL, 'u'},
        {"invert",           no_argument, NULL, 'i'},
        {"brightness", required_argument, NULL, 'b'},
        {NULL}
    };

    std::string configName = "";
    std::string displayName = "";
    std::string skinFileName = "";
    bool upsideDown = false;
    bool invert = false;
    int brightness = -1;
    unsigned int displayNumber = 0;

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "c:d:s:uib:", long_options, &option_index)) != -1)
    {
        switch (c)
        {
            case 'c':
                configName = optarg;
                break;

            case 'd':
                displayName = optarg;
                break;

            case 's':
                skinFileName = optarg;
                break;

            case 'u':
                upsideDown = true;
                break;

            case 'i':
                invert = true;
                break;

            case 'b':
                brightness = atoi(optarg);
                if (brightness < 0) brightness = 0;
                if (brightness > 100) brightness = 100;
                break;

            default:
                //usage();
                return 1;
        }
    }

    if (configName.length() == 0)
    {
        configName = kDefaultConfigFile;
        fprintf(stdout, "WARNING: No config file specified, using default (%s).\n", configName.c_str());
    }

    if (GLCD::Config.Load(configName) == false)
    {
        fprintf(stderr, "Error loading config file!\n");
        return 1;
    }
    if (GLCD::Config.driverConfigs.size() > 0)
    {
        if (displayName.length() > 0)
        {
            for (displayNumber = 0; displayNumber < GLCD::Config.driverConfigs.size(); displayNumber++)
            {
                if (GLCD::Config.driverConfigs[displayNumber].name == displayName)
                    break;
            }
            if (displayNumber == GLCD::Config.driverConfigs.size())
            {
                fprintf(stderr, "ERROR: Specified display %s not found in config file!\n", displayName.c_str());
                return 1;
            }
        }
        else
        {
            fprintf(stdout, "WARNING: No display specified, using first one.\n");
            displayNumber = 0;
        }
    }
    else
    {
        fprintf(stderr, "ERROR: No displays specified in config file!\n");
        return 1;
    }

    GLCD::Config.driverConfigs[displayNumber].upsideDown ^= upsideDown;
    GLCD::Config.driverConfigs[displayNumber].invert ^= invert;
    if (brightness != -1)
        GLCD::Config.driverConfigs[displayNumber].brightness = brightness;

    GLCD::cDriver * lcd = GLCD::CreateDriver(GLCD::Config.driverConfigs[displayNumber].id, &GLCD::Config.driverConfigs[displayNumber]);
    if (!lcd)
    {
        fprintf(stderr, "ERROR: Failed creating display object %s\n", displayName.c_str());
        return 9;
    }
    if (lcd->Init() != 0)
    {
        fprintf(stderr, "ERROR: Failed initializing display %s\n", displayName.c_str());
        delete lcd;
        return 10;
    }

    GLCD::cBitmap * screen = new GLCD::cBitmap(lcd->Width(), lcd->Height());
    screen->Clear();

    cMySkinConfig skinConfig(lcd);
    GLCD::cSkin * skin = GLCD::XmlParse(skinConfig, "test", skinFileName);
    skin->SetBaseSize(screen->Width(), screen->Height());
    GLCD::cSkinDisplay * display = skin->GetDisplay("normal");

    display->Render(screen);
    lcd->SetScreen(screen->Data(), screen->Width(), screen->Height());
    lcd->Refresh(true);

    lcd->DeInit();
    delete lcd;

    return 0;
}
