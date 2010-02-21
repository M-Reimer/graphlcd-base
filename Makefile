#
# Makefile for the GraphLCD driver library, graphics library and tools
#

PROJECT = graphlcd-base
VERSION = 0.1.5
ARCHIVE = $(PROJECT)-$(VERSION)
PACKAGE = $(ARCHIVE)
TMPDIR = /tmp

### Targets:

all:
	@$(MAKE) -C glcdgraphics all
	@$(MAKE) -C glcddrivers all
	@$(MAKE) -C tools all

install:
	@$(MAKE) -C glcdgraphics install
	@$(MAKE) -C glcddrivers install
	@$(MAKE) -C tools install

uninstall:
	@$(MAKE) -C glcdgraphics uninstall
	@$(MAKE) -C glcddrivers uninstall
	@$(MAKE) -C tools uninstall
  
clean:
	@-rm -f *.tgz
	@$(MAKE) -C glcdgraphics clean
	@$(MAKE) -C glcddrivers clean
	@$(MAKE) -C tools clean

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude .svn --exclude *.cbp --exclude *.layout -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

