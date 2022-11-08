#
# Makefile for the GraphLCD tool showtext
#

include ../../Make.config

PRGNAME = showtext

OBJS = showtext.o

INCLUDES += -I../../
LIBDIRS += -L../../glcdgraphics/ -L../../glcddrivers/


all: $(PRGNAME)
.PHONY: all

# Implicit rules:

%.o: %.cpp
	$(CXX) $(CXXEXTRA) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

DEPFILE = $(OBJS:%.o=%.d)

-include $(DEPFILE)

# The main program:

$(PRGNAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -rdynamic $(OBJS) $(LIBS) $(LIBDIRS) -lglcdgraphics -lglcddrivers -lstdc++ -o $(PRGNAME)

install: $(PRGNAME)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(HAVE_STRIP) $(PRGNAME) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(PRGNAME)

clean:
	@-rm -f $(OBJS) $(DEPFILE) $(PRGNAME) *~
