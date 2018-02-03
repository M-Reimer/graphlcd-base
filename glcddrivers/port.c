/*
 * GraphLCD driver library
 *
 * port.c  -  parallel port class with low level routines
 *
 * This file is released under the GNU General Public License. Refer
 * to the COPYING file distributed with this package.
 *
 * (c) 2004      Andreas Regel <andreas.regel AT powarman.de>
 * (c) 2011-2012 Wolfgang Astleitner <mrwastl AT users.sourceforge.net>
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include <linux/parport.h>



#include "port.h"

#if defined(__linux__) && (defined(__i386__) || defined(__x86_64__))
  #define __HAS_DIRECTIO__ 1
#endif

namespace GLCD
{

static pthread_mutex_t claimport_mutex;

static inline int port_in(int port)
{
#ifdef __HAS_DIRECTIO__
    unsigned char value;
    __asm__ volatile ("inb %1,%0"
                      : "=a" (value)
                      : "d" ((unsigned short) port));
    return value;
#else
    return 0;
#endif
}

static inline void port_out(unsigned short int port, unsigned char val)
{
#ifdef __HAS_DIRECTIO__
    __asm__ volatile ("outb %0,%1\n"
                      :
                      : "a" (val), "d" (port));
#endif
}

cParallelPort::cParallelPort()
:   fd(-1),
    port(0),
    usePPDev(false),
    portClaimed(false)
{
}

cParallelPort::~cParallelPort()
{
}

int cParallelPort::Open(int portIO)
{
#ifdef __HAS_DIRECTIO__
    usePPDev = false;
    port = portIO;

    if (port < 0x400)
    {
        if (ioperm(port, 3, 255) == -1)
        {
            syslog(LOG_ERR, "glcd drivers: ERROR ioperm(0x%X) failed! Err:%s (cParallelPort::Open)\n",
                   port, strerror(errno));
            return -1;
        }
    }
    else
    {
        if (iopl(3) == -1)
        {
            syslog(LOG_ERR, "glcd drivers: ERROR iopl failed! Err:%s (cParallelPort::Init)\n",
                   strerror(errno));
            return -1;
        }
    }
    return 0;
#else
    syslog(LOG_ERR, "glcd drivers: ERROR: direct IO/parport is not available on this architecture / operating system\n");
    return -1;
#endif
}

int cParallelPort::Open(const char * device)
{
    usePPDev = true;

    fd = open(device, O_RDWR);
    if (fd == -1)
    {
        syslog(LOG_ERR, "glcd drivers: ERROR cannot open %s. Err:%s (cParallelPort::Init)\n",
               device, strerror(errno));
        return -1;
    }

    if (!Claim())
    {
        syslog(LOG_ERR, "glcd drivers: ERROR cannot claim %s. Err:%s (cParallelPort::Init)\n",
               device, strerror(errno));
        close(fd);
        return -1;
    }

    int mode = PARPORT_MODE_PCSPP;
    if (ioctl(fd, PPSETMODE, &mode) != 0)
    {
        syslog(LOG_ERR, "glcd drivers: ERROR cannot setmode %s. Err:%s (cParallelPort::Init)\n",
               device, strerror(errno));
        close(fd);
        return -1;
    }

    return 0;
}

int cParallelPort::Close()
{
    if (usePPDev)
    {
        if (fd != -1)
        {
            ioctl(fd, PPRELEASE);
            close(fd);
            fd = -1;
        }
        else
        {
            return -1;
        }
    }
    else
    {
#ifdef __HAS_DIRECTIO__
        if (port < 0x400)
        {
            if (ioperm(port, 3, 0) == -1)
            {
                return -1;
            }
        }
        else
        {
            if (iopl(0) == -1)
            {
                return -1;
            }
        }
#else
        return -1;  // should never make it until here ...
#endif
    }
    return 0;
}

bool cParallelPort::Claim()
{
    if (!IsPortClaimed())
    {
        if (usePPDev)
            portClaimed = (ioctl(fd, PPCLAIM) == 0);
        else
            portClaimed = (pthread_mutex_lock(&claimport_mutex) == 0);
    }
    return IsPortClaimed();
}

void cParallelPort::Release()
{
    if (IsPortClaimed())
    {
        if (usePPDev)
            portClaimed = !(ioctl(fd, PPRELEASE) == 0);
        else
            portClaimed = !(pthread_mutex_unlock(&claimport_mutex) == 0);
    }
}

void cParallelPort::SetDirection(int direction)
{
    if (usePPDev)
    {
        if (ioctl(fd, PPDATADIR, &direction) == -1)
        {
            perror("ioctl(PPDATADIR)");
            //exit(1);
        }
    }
    else
    {
        if (direction == kForward)
            port_out(port + 2, port_in(port + 2) & 0xdf);
        else
            port_out(port + 2, port_in(port + 2) | 0x20);
    }
}

unsigned char cParallelPort::ReadControl()
{
    unsigned char value;

    if (usePPDev)
    {
        if (ioctl(fd, PPRCONTROL, &value) == -1)
        {
            perror("ioctl(PPRCONTROL)");
            //exit(1);
        }
    }
    else
    {
        value = port_in(port + 2);
    }

    return value;
}

void cParallelPort::WriteControl(unsigned char value)
{
    if (usePPDev)
    {
        if (ioctl(fd, PPWCONTROL, &value) == -1)
        {
            perror("ioctl(PPWCONTROL)");
            //exit(1);
        }
    }
    else
    {
        port_out(port + 2, value);
    }
}

unsigned char cParallelPort::ReadStatus()
{
    unsigned char value;

    if (usePPDev)
    {
        if (ioctl(fd, PPRSTATUS, &value) == -1)
        {
            perror("ioctl(PPRSTATUS)");
            //exit(1);
        }
    }
    else
    {
        value = port_in(port + 1);
    }

    return value;
}

unsigned char cParallelPort::ReadData()
{
    unsigned char data;

    if (usePPDev)
    {
        if (ioctl(fd, PPRDATA, &data) == -1)
        {
            perror("ioctl(PPRDATA)");
            //exit(1);
        }
    }
    else
    {
        data = port_in(port);
    }

    return data;
}

void cParallelPort::WriteData(unsigned char data)
{
    if (usePPDev)
    {
        if (ioctl(fd, PPWDATA, &data) == -1)
        {
            perror("ioctl(PPWDATA)");
            //exit(1);
        }
    }
    else
    {
        port_out(port, data);
    }
}



cSerialPort::cSerialPort()
:   fd(-1)
{
}

cSerialPort::~cSerialPort()
{
}

int cSerialPort::Open(const char * device)
{
    struct termios options;

    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        printf("error opening port\n");
        return -1;
    }
    //fcntl(fd, F_SETFL, FNDELAY);
    fcntl(fd, F_SETFL, 0);

    tcgetattr(fd, &options);

    cfsetispeed(&options, B921600);
    cfsetospeed(&options, B921600);

    // 8 bits, no parity, no stop bits
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // No hardware flow control
    options.c_cflag &= ~CRTSCTS;

    // Enable receiver, ignore status lines
    options.c_cflag |= (CLOCAL | CREAD);

    // disable canonical input, disable echo,
    // disable visually erase chars
    // disable terminal-generated signals
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // disable input/output flow control, disable restart chars
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    // disable output processing
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);

    return 0;
}

int cSerialPort::Close()
{
    if (fd == -1)
        return -1;
    close(fd);
    return 0;
}

void cSerialPort::SetBaudRate(int speed)
{
    struct termios2 tio;
    if (ioctl(fd, TCGETS2, &tio) < 0)
    {
        printf("TCGETS2 ioctl failed!\n");
        return;
    }
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = speed;
    tio.c_ospeed = speed;
    if (ioctl(fd, TCSETS2, &tio) < 0)
    {
        printf("TCSETS2 ioctl failed!\n");
        return;
    }
}

// Configures the serial port to not send a "hangup" signal.
// Returns true if the flag was set and had to be removed and false otherwise.
bool cSerialPort::DisableHangup()
{
    struct termios options;
    tcgetattr(fd, &options);
    if (!(options.c_cflag & HUPCL))
        return false;
    options.c_cflag &= ~HUPCL;
    tcsetattr(fd, TCSANOW, &options);
    return true;
}

int cSerialPort::ReadData(unsigned char * data)
{
    if (fd == -1)
        return 0;
    return read(fd, data, 1);
}

void cSerialPort::WriteData(unsigned char data)
{
    WriteData(&data, 1);
}

void cSerialPort::WriteData(unsigned char * data, unsigned short length)
{
    if (fd == -1)
        return;
    write(fd, data, length);
}

void cSerialPort::WriteData(std::string data) {
    WriteData((unsigned char*)data.c_str(), data.length());
}

} // end of namespace
