#
# Makefile for the GraphLCD graphics library
#

include ../Make.config

# External image lib to use: imagemagick, graphicsmagick, or none
# (two ifdef/endif are used because older installations may not support 'else ifdef')
IMAGELIB =
ifdef HAVE_GRAPHICSMAGICK
	IMAGELIB = graphicsmagick
endif
ifdef HAVE_IMAGEMAGICK
	IMAGELIB = imagemagick
	HAVE_IMAGEMAGICK_7 = $(shell pkg-config --atleast-version=7.0.0 MagickWand && echo true)
endif


CXXFLAGS += -fPIC

VERMAJOR = 2
VERMINOR = 1
VERMICRO = 0

BASENAME = libglcdgraphics.so

LIBNAME = $(BASENAME).$(VERMAJOR).$(VERMINOR).$(VERMICRO)

OBJS = bitmap.o common.o font.o glcd.o image.o imagefile.o pbm.o extformats.o

HEADERS = bitmap.h font.h glcd.h image.h imagefile.h pbm.h extformats.h

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXEXTRA) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

DEPFILE = $(OBJS:%.o=%.d)

-include $(DEPFILE)

ifdef HAVE_FREETYPE2
	DEFINES += -DHAVE_FREETYPE2
	INCLUDES += $(shell pkg-config freetype2 --cflags)
	LIBS += $(shell pkg-config freetype2 --libs)
endif

# two ifdef/endif are used because older installations may not support 'else ifdef'
ifeq ($(IMAGELIB), imagemagick)
	DEFINES += -DHAVE_IMAGEMAGICK
	INCLUDES += $(shell pkg-config --cflags MagickWand)
	LIBS += $(shell pkg-config --libs MagickWand)
	ifeq ($(HAVE_IMAGEMAGICK_7), true)
		DEFINES += -DHAVE_IMAGEMAGICK_7
	endif
endif

ifeq ($(IMAGELIB), graphicsmagick)
	DEFINES += -DHAVE_IMAGEMAGICK # yep, really HAVE_IMAGEMAGICK here
	INCLUDES += $(shell pkg-config --cflags GraphicsMagickWand)
	LIBS += $(shell pkg-config --libs GraphicsMagickWand)
endif


### Targets:

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -Wl,-soname="$(BASENAME).$(VERMAJOR)" -Wl,--no-undefined -o $@
	ln -sf $(LIBNAME) $(BASENAME)

install: all
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(LIBNAME) $(DESTDIR)$(LIBDIR)/
	install -d $(DESTDIR)$(INCDIR)/glcdgraphics
	install -m 644 $(HEADERS) $(DESTDIR)$(INCDIR)/glcdgraphics/
	( cd $(DESTDIR)$(LIBDIR); ln -sf $(LIBNAME) $(BASENAME).$(VERMAJOR); ln -sf $(LIBNAME) $(BASENAME) )

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(BASENAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(BASENAME).$(VERMAJOR)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBNAME)
	(for i in $(HEADERS); do rm -f $(DESTDIR)$(INCDIR)/glcdgraphics/$$i; done)
	rmdir $(DESTDIR)$(INCDIR)/glcdgraphics

clean:
	rm -f $(OBJS) $(DEPFILE) $(LIBNAME) $(BASENAME) *~

