#
# Makefile for the GraphLCD tool crtfont
#

-include ../../Make.config

PRGNAME = genfont

OBJS = genfont.o

INCLUDES += -I../../
INCLUDES += `freetype-config --cflags`

LIBDIRS += -L../../glcdgraphics/
LIBS += `freetype-config --libs`

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
	install -d $(BINDIR)
	install -m 755 -o root -g root $(HAVE_STRIP) $(PRGNAME) $(BINDIR)
	
uninstall:
	rm -f $(BINDIR)/$(PRGNAME)

clean:
	@-rm -f $(OBJS) $(DEPFILE) $(PRGNAME) *~

