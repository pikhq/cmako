EXTRA_DIST = COPYING

bin_PROGRAMS = mako

mako_SOURCES = src/main.c src/mako-vm.c src/draw.c include/ui.h \
	       include/draw.h include/mako-vm.h include/constants.h

AM_CFLAGS = -I$(srcdir)/include @SDL_CFLAGS@ @SDL_GFX_CFLAGS@
mako_LDADD = @SDL_LIBS@ @SDL_GFX_LIBS@
