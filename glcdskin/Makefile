#
# Makefile for the GraphLCD skin library
#

include ../Make.config

CXXFLAGS += -fPIC

VERMAJOR = 2
VERMINOR = 1
VERMICRO = 0

BASENAME = libglcdskin.so

LIBNAME = $(BASENAME).$(VERMAJOR).$(VERMINOR).$(VERMICRO)

OBJS = cache.o config.o display.o font.o function.o object.o parser.o skin.o string.o type.o variable.o xml.o

HEADERS = cache.h config.h display.h font.h function.h object.h parser.h skin.h string.h type.h variable.h xml.h


### Inner graphlcd-base dependencies
LIBS += -L../glcdgraphics -lglcdgraphics -L../glcddrivers -lglcddrivers

ifdef HAVE_FONTCONFIG
  LIBS +=  -lfontconfig
  DEFINES += -DHAVE_FONTCONFIG
endif

### Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXEXTRA) $(CXXFLAGS) -I.. -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

DEPFILE = $(OBJS:%.o=%.d)

-include $(DEPFILE)

### Targets:

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $(OBJS) $(LIBS) -Wl,-soname="$(BASENAME).$(VERMAJOR)" -Wl,--no-undefined -o $@
	ln -sf $(LIBNAME) $(BASENAME)

install: all
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(LIBNAME) $(DESTDIR)$(LIBDIR)/
	install -d $(DESTDIR)$(INCDIR)/glcdskin
	install -m 644 $(HEADERS) $(DESTDIR)$(INCDIR)/glcdskin/
	( cd $(DESTDIR)$(LIBDIR); ln -sf $(LIBNAME) $(BASENAME).$(VERMAJOR); ln -sf $(LIBNAME) $(BASENAME) )

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(BASENAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(BASENAME).$(VERMAJOR)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIBNAME)
	(for i in $(HEADERS); do rm -f $(DESTDIR)$(INCDIR)/glcdskin/$$i; done)
	rmdir $(DESTDIR)$(INCDIR)/glcdskin

clean:
	rm -f $(OBJS) $(DEPFILE) $(LIBNAME) $(BASENAME) *~

