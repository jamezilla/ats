## Process this file with automake to create Makefile.in
noinst_LIBRARIES = libatsa.a
libatsa_a_DEPENDENCIES = src/sndlib/sndlib.a
libatsa_a_SOURCES = src/atsa/atsa.h src/atsa/critical-bands.c src/atsa/other-utils.c src/atsa/peak-detection.c src/atsa/peak-tracking.c src/atsa/residual-analysis.c src/atsa/residual.c src/atsa/save-load-sound.c src/atsa/tracker.c src/atsa/utilities.c
libatsa_a_LIBADD = src/sndlib/sndlib.a
libatsa_a_CFLAGS = -Isrc/sndlib

bin_PROGRAMS = atsa atsh

atsa_SOURCES = src/atsa/atsa-nogui.c src/atsa/atsa.c
atsa_LDADD = libatsa.a
atsa_CFLAGS = -Wall -Isrc/sndlib

atsh_SOURCES = src/atsh/about.c src/atsh/analysis.c src/atsh/atsh.c src/atsh/curves.c src/atsh/header.c src/atsh/files.c src/atsh/graphics.c src/atsh/atsh.h src/atsh/mask.c src/atsh/sel.c src/atsh/undo.c src/atsh/get_sparams.c src/atsh/list_view.c src/atsh/my_curve.c src/atsh/my_curve.h src/atsh/popup.c src/atsh/progress.c src/atsh/synth-funcs.c src/atsh/help.c src/atsh/help.h
atsh_LDADD = libatsa.a @GTK_LIBS@
atsh_CFLAGS = -Wall -Isrc/sndlib -Isrc/atsa @GTK_CFLAGS@

src/sndlib/sndlib.a:
	cd src/sndlib ; ./configure --with-doubles ; make

sclean:
	cd src/sndlib ; make clean

dist-hook:
	rm -rf `find $(distdir)/doc -name CVS`

EXTRA_DIST = README TODO doc \
src/sndlib/clm.c src/sndlib/clm.h src/sndlib/clm-strings.h src/sndlib/config.guess src/sndlib/config.h.in src/sndlib/config.sub src/sndlib/configure src/sndlib/headers.c src/sndlib/install-sh src/sndlib/io.c src/sndlib/makefile.in src/sndlib/README src/sndlib/sndlib-config.in src/sndlib/sndlib.h src/sndlib/sndlib-strings.h src/sndlib/sound.c
#DIST_SUBDIRS = examples doc
