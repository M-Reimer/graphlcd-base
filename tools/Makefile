#
# Makefile for the graphlcd tools
#


-include ../Make.config

### Targets:

all:
	@$(MAKE) -C convpic all
	@$(MAKE) -C crtfont all
ifdef HAVE_FREETYPE2
	@$(MAKE) -C genfont all
endif
	@$(MAKE) -C showpic all
	@$(MAKE) -C showtext all
	@$(MAKE) -C lcdtestpattern all

install:
	@$(MAKE) -C convpic install
	@$(MAKE) -C crtfont install
ifdef HAVE_FREETYPE2
	@$(MAKE) -C genfont install
endif
	@$(MAKE) -C showpic install
	@$(MAKE) -C showtext install
	@$(MAKE) -C lcdtestpattern install

uninstall:
	@$(MAKE) -C convpic uninstall
	@$(MAKE) -C crtfont uninstall
ifdef HAVE_FREETYPE2
	@$(MAKE) -C genfont uninstall
endif
	@$(MAKE) -C showpic uninstall
	@$(MAKE) -C showtext uninstall
	@$(MAKE) -C lcdtestpattern uninstall
 
clean:
	@$(MAKE) -C convpic clean
	@$(MAKE) -C crtfont clean
	@$(MAKE) -C genfont clean
	@$(MAKE) -C showpic clean
	@$(MAKE) -C showtext clean
	@$(MAKE) -C lcdtestpattern clean

