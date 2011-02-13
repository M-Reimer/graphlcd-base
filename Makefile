#
# Makefile for the GraphLCD driver library, graphics library and tools
#

PROJECT = graphlcd-base
VERSION = 0.1.9
ARCHIVE = $(PROJECT)-$(VERSION)
PACKAGE = $(ARCHIVE)
TMPDIR = /tmp

INCLUDE_SKINS=1

### Targets:

all:
	@$(MAKE) -C glcdgraphics all
	@$(MAKE) -C glcddrivers all
ifdef INCLUDE_SKINS
	@$(MAKE) -C glcdskin all
endif
	@$(MAKE) -C tools all

install:
	@$(MAKE) -C glcdgraphics install
	@$(MAKE) -C glcddrivers install
ifdef INCLUDE_SKINS
	@$(MAKE) -C glcdskin install
endif
	@$(MAKE) -C tools install

uninstall:
	@$(MAKE) -C glcdgraphics uninstall
	@$(MAKE) -C glcddrivers uninstall
ifdef INCLUDE_SKINS
	@$(MAKE) -C glcdskin uninstall
endif
	@$(MAKE) -C tools uninstall
  
clean:
	@-rm -f *.tgz
	@$(MAKE) -C glcdgraphics clean
	@$(MAKE) -C glcddrivers clean
ifdef INCLUDE_SKINS
	@$(MAKE) -C glcdskin clean
endif
	@$(MAKE) -C tools clean

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude .svn --exclude *.cbp --exclude *.layout -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

