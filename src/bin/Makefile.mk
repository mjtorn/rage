bin_PROGRAMS = src/bin/rage

src_bin_rage_CPPFLAGS = -I. \
-DPACKAGE_BIN_DIR=\"$(bindir)\" -DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" @RAGE_CFLAGS@

src_bin_rage_LDADD = @RAGE_LIBS@

src_bin_rage_SOURCES = \
src/bin/albumart.c \
src/bin/albumart.h \
src/bin/browser.c \
src/bin/browser.h \
src/bin/config.c \
src/bin/config.h \
src/bin/controls.c \
src/bin/controls.h \
src/bin/dnd.c \
src/bin/dnd.h \
src/bin/gesture.c \
src/bin/gesture.h \
src/bin/key.c \
src/bin/key.h \
src/bin/main.c \
src/bin/main.h \
src/bin/sha1.c \
src/bin/sha1.h \
src/bin/video.c \
src/bin/video.h \
src/bin/videothumb.c \
src/bin/videothumb.h \
src/bin/win.c \
src/bin/win.h \
src/bin/winlist.c \
src/bin/winlist.h \
src/bin/winvid.c \
src/bin/winvid.h

internal_bindir = $(libdir)/rage/utils
internal_bin_PROGRAMS = src/bin/rage_thumb

src_bin_rage_thumb_SOURCES = \
src/bin/thumb.c \
src/bin/albumart.c \
src/bin/albumart.h \
src/bin/sha1.c \
src/bin/sha1.h

src_bin_rage_thumb_LDADD = @RAGE_LIBS@
src_bin_rage_thumb_CPPFLAGS = -Isrc/bin \
-DPACKAGE_BIN_DIR=\"$(bindir)\" -DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DPACKAGE_DATA_DIR=\"$(pkgdatadir)\" @RAGE_CFLAGS@
