#
# Makefile for the GraphLCD tool showtext
#

-include ../../Make.config

PRGNAME = skintest

OBJS = skintest.o

INCLUDES += -I../../
LIBDIRS += -L../../glcdgraphics/ -L../../glcddrivers/ -L../../glcdskin/


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
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -rdynamic $(OBJS) $(LIBS) $(LIBDIRS) -lglcdgraphics -lglcddrivers -lglcdskin -lstdc++ -o $(PRGNAME)

install: $(PRGNAME)
	install -d $(BINDIR)
	install -m 755 -o root -g root $(HAVE_STRIP) $(PRGNAME) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(PRGNAME)

clean:
	@-rm -f $(OBJS) $(DEPFILE) $(PRGNAME) *~