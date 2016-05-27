AUTOMAKE_OPTIONS = subdir-objects
MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = rage

rage_CPPFLAGS = -I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" -DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" @RAGE_CFLAGS@

rage_LDADD = @RAGE_LIBS@

rage_SOURCES = \
controls.c controls.h \
dnd.c dnd.h \
gesture.c gesture.h \
key.c key.h \
main.c main.h \
video.c video.h \
winvid.c winvid.h \
win.c win.h \
winlist.c winlist.h \
config.c config.h \
sha1.c sha1.h \
videothumb.c videothumb.h \
albumart.c albumart.h \
browser.c browser.h


internal_bindir = $(libdir)/rage/utils
internal_bin_PROGRAMS = rage_thumb

rage_thumb_SOURCES = \
thumb.c \
sha1.c sha1.h \
albumart.c albumart.h

rage_thumb_LDADD = @RAGE_LIBS@
rage_thumb_CPPFLAGS = -I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" -DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" @RAGE_CFLAGS@
