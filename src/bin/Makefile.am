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
winlist.c winlist.h
