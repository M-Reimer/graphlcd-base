#
# Makefile for the GraphLCD graphics library
#

-include ../Make.config

# External image lib to use: imagemagick, graphicsmagick, or none
# (two ifdef/endif are used because older installations may not support 'else ifdef')
IMAGELIB = 
ifdef HAVE_GRAPHICSMAGICK
	IMAGELIB = graphicsmagick
endif
ifdef HAVE_IMAGEMAGICK
	IMAGELIB = imagemagick
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

%.o: %.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = g++ -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

ifdef HAVE_FREETYPE2
	ifneq ($(shell which freetype-config),)
		INCLUDES += $(shell freetype-config --cflags)
		LIBS += $(shell freetype-config --libs)
	else
		INCLUDES += -I/usr/include/freetype -I/usr/local/include/freetype
		LIBS += -lfreetype
	endif
	DEFINES += -DHAVE_FREETYPE2
endif### Targets:


# two ifdef/endif are used because older installations may not support 'else ifdef'
ifeq ($(IMAGELIB), imagemagick)
	DEFINES += -DHAVE_IMAGEMAGICK
	INCLUDES += $(shell pkg-config --cflags ImageMagick++)
	LIBS += $(shell pkg-config --libs ImageMagick++)
endif
ifeq ($(IMAGELIB), graphicsmagick)
	DEFINES += -DHAVE_IMAGEMAGICK # yep, really HAVE_IMAGEMAGICK here
	INCLUDES += $(shell pkg-config --cflags GraphicsMagick++)
	LIBS += $(shell pkg-config --libs GraphicsMagick++)
endif


all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -Wl,-soname="$(BASENAME).$(VERMAJOR)" -o $@
	ln -sf $(LIBNAME) $(BASENAME)

install: all
	install -d $(LIBDIR)
	install -m 755 $(LIBNAME) $(LIBDIR)/
	install -d $(INCDIR)/glcdgraphics
	install -m 644 $(HEADERS) $(INCDIR)/glcdgraphics/
	( cd $(LIBDIR); ln -sf $(LIBNAME) $(BASENAME).$(VERMAJOR); ln -sf $(LIBNAME) $(BASENAME) )

uninstall:
	rm -f $(LIBDIR)/$(BASENAME)
	rm -f $(LIBDIR)/$(BASENAME).$(VERMAJOR)
	rm -f $(LIBDIR)/$(LIBNAME)
	(for i in $(HEADERS); do rm -f $(INCDIR)/glcdgraphics/$$i; done)
	rmdir $(INCDIR)/glcdgraphics

clean:
	rm -f $(OBJS) $(DEPFILE) $(LIBNAME) $(BASENAME) *~

