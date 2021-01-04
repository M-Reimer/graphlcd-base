#
# Makefile for the GraphLCD driver library
#

include ../Make.config

CXXFLAGS += -fPIC

VERMAJOR = 2
VERMINOR = 1
VERMICRO = 0

BASENAME = libglcddrivers.so

LIBNAME = $(BASENAME).$(VERMAJOR).$(VERMINOR).$(VERMICRO)

OBJS = common.o config.o driver.o drivers.o port.o simlcd.o framebuffer.o gu140x32f.o gu256x64-372.o gu256x64-3900.o hd61830.o ks0108.o image.o sed1330.o sed1520.o t6963c.o noritake800.o serdisp.o avrctl.o g15daemon.o network.o gu126x64D-K610A4.o dm140gink.o usbserlcd.o st7565r-reel.o

HEADERS = config.h driver.h drivers.h

ifeq ($(shell pkg-config --exists libhid && echo 1), 1)
OBJS += futabaMDM166A.o
INCLUDES += $(shell pkg-config --cflags libhid)
LIBS += $(shell pkg-config --libs libhid)
DEFINES += -DHAVE_LIBHID
endif


ifeq ($(shell pkg-config --exists libusb && echo 1), 1)
    DEFINES += -DHAVE_LIBUSB
    ifdef HAVE_DRIVER_AX206DPF
        OBJS += ax206dpf.o
        INCLUDES += $(shell pkg-config --cflags libusb)
        LIBS += $(shell pkg-config --libs libusb)
        DEFINES += -DHAVE_DRIVER_AX206DPF
    endif
    ifdef HAVE_DRIVER_picoLCD_256x64
        OBJS += picoLCD_256x64.o
        INCLUDES += $(shell pkg-config --cflags libusb)
        LIBS += $(shell pkg-config --libs libusb)
        DEFINES += -DHAVE_DRIVER_picoLCD_256x64
    endif
endif

ifeq ($(HAVE_DRIVER_VNCSERVER), 1)
ifeq ($(shell pkg-config --exists libvncserver && echo 1), 1)
    DEFINES += -DHAVE_DRIVER_VNCSERVER
    OBJS += vncserver.o
    INCLUDES += $(shell pkg-config --cflags libvncserver)
    LIBS += $(shell pkg-config --libs libvncserver)
endif
endif

ifeq ($(HAVE_DRIVER_SSD1306), 1)
    DEFINES += -DHAVE_DRIVER_SSD1306
    OBJS += ssd1306.o
    LIBS += -lwiringPi
endif

ifeq ($(HAVE_DRIVER_ILI9341), 1)
    DEFINES += -DHAVE_DRIVER_ILI9341
    OBJS += ili9341.o
    LIBS += -lwiringPi
endif

### Implicit rules:

%.o: %.c
	$(CXX) $(CXXEXTRA) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

DEPFILE = $(OBJS:%.o=%.d)

-include $(DEPFILE)

### Targets:

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -ldl -lpthread -Wl,-soname="$(BASENAME).$(VERMAJOR)" -Wl,--no-undefined -o $@
	ln -sf $(LIBNAME) $(BASENAME)

install: all
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(LIBNAME) $(DESTDIR)$(LIBDIR)/
	install -d $(DESTDIR)$(INCDIR)/glcddrivers
	install -m 644 $(HEADERS) $(DESTDIR)$(INCDIR)/glcddrivers/
	( cd $(DESTDIR)$(LIBDIR); ln -sf $(LIBNAME) $(BASENAME).$(VERMAJOR); ln -sf $(LIBNAME) $(BASENAME) )

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(BASENAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(BASENAME).$(VERMAJOR)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBNAME)
	(for i in $(HEADERS); do rm -f $(DESTDIR)$(INCDIR)/glcddrivers/$$i; done)
	rmdir $(DESTDIR)$(INCDIR)/glcddrivers

clean:
	rm -f $(OBJS) $(DEPFILE) $(LIBNAME) $(BASENAME) *~

