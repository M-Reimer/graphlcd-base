#
# Makefile for the GraphLCD driver library, graphics library and tools
#

PROJECT = graphlcd-base
VERSION = 0.3.0
ARCHIVE = $(PROJECT)-$(VERSION)
PACKAGE = $(ARCHIVE)
TMPDIR = /tmp

UDEVRULESDIR ?= /etc/udev/rules.d/
UDEVRULE ?= 99-graphlcd-base.rules

### Targets:

all:
	@$(MAKE) -C glcdgraphics all
	@$(MAKE) -C glcddrivers all
	@$(MAKE) -C glcdskin all
	@$(MAKE) -C tools all

install:
	@$(MAKE) -C glcdgraphics install
	@$(MAKE) -C glcddrivers install
	@$(MAKE) -C glcdskin install
	@$(MAKE) -C tools install
	test -d "${UDEVRULESDIR}" && install -m 644 -o root -g root "$(UDEVRULE)" "$(UDEVRULESDIR)"

uninstall:
	@$(MAKE) -C glcdgraphics uninstall
	@$(MAKE) -C glcddrivers uninstall
	@$(MAKE) -C glcdskin uninstall
	@$(MAKE) -C tools uninstall
  
clean:
	@-rm -f *.tgz
	@$(MAKE) -C glcdgraphics clean
	@$(MAKE) -C glcddrivers clean
	@$(MAKE) -C glcdskin clean
	@$(MAKE) -C tools clean

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(PACKAGE).tgz --exclude .svn --exclude *.cbp --exclude *.layout -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(PACKAGE).tgz

