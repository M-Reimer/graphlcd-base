#
# Makefile for the GraphLCD tool crtfont
#

include ../../Make.config

PRGNAME = genfont

OBJS = genfont.o

INCLUDES += -I../../
INCLUDES += $(shell pkg-config freetype2 --cflags)

LIBDIRS += -L../../glcdgraphics/
LIBS += $(shell pkg-config freetype2 --libs)

all: $(PRGNAME)
.PHONY: all

# Implicit rules:

%.o: %.c
	$(CXX) $(CXXEXTRA) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

DEPFILE = $(OBJS:%.o=%.d)

-include $(DEPFILE)

# The main program:

$(PRGNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -rdynamic $(OBJS) $(LIBDIRS) $(LIBS) -lglcdgraphics -lstdc++ -o $(PRGNAME)

install: $(PRGNAME)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(HAVE_STRIP) $(PRGNAME) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PRGNAME)

clean:
	@-rm -f $(OBJS) $(DEPFILE) $(PRGNAME) *~
