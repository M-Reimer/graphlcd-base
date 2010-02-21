/*
 * GraphLCD driver library
 *
 * port.h  -  parallel port class with low level routines
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004 Andreas Regel <andreas.regel AT powarman.de>
 */

#ifndef _GLCDDRIVERS_PORT_H_
#define _GLCDDRIVERS_PORT_H_

namespace GLCD
{

const int kForward = 0;
const int kReverse = 1;

const unsigned char kStrobeHigh = 0x00; // Pin 1
const unsigned char kStrobeLow  = 0x01;
const unsigned char kAutoHigh   = 0x00; // Pin 14
const unsigned char kAutoLow    = 0x02;
const unsigned char kInitHigh   = 0x04; // Pin 16
const unsigned char kInitLow    = 0x00;
const unsigned char kSelectHigh = 0x00; // Pin 17
const unsigned char kSelectLow  = 0x08;

class cParallelPort
{
private:
    int fd;
    int port;
    bool usePPDev;

public:
    cParallelPort();
    ~cParallelPort();

    int Open(int port);
    int Open(const char * device);
    int Close();

    bool IsDirectIO() const { return (!usePPDev); }
    int GetPortHandle() const { return ((usePPDev) ? fd : port); }

    void Claim();
    void Release();

    void SetDirection(int direction);
    unsigned char ReadControl();
    void WriteControl(unsigned char values);
    unsigned char ReadStatus();
    unsigned char ReadData();
    void WriteData(unsigned char data);
};

class cSerialPort
{
private:
    int fd;

public:
    cSerialPort();
    ~cSerialPort();

    int Open(const char * device);
    int Close();

    int ReadData(unsigned char * data);
    void WriteData(unsigned char data);
    void WriteData(unsigned char * data, unsigned short length);
};

} // end of namespace

#endif
