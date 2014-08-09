EDJE_CC = @edje_cc@
EDJE_FLAGS_VERBOSE_ =
EDJE_FLAGS_VERBOSE_0 =
EDJE_FLAGS_VERBOSE_1 = -v
EDJE_FLAGS = $(EDJE_FLAGS_VERBOSE_$(V)) -id $(top_srcdir)/data/themes/images

themedir = $(pkgdatadir)/themes
theme_DATA = \
data/themes/default.edj

include data/themes/images/Makefile.mk

AM_V_EDJ = $(am__v_EDJ_$(V))
am__v_EDJ_ = $(am__v_EDJ_$(AM_DEFAULT_VERBOSITY))
am__v_EDJ_0 = @echo "  EDJ   " $@;

EXTRA_DIST += \
data/themes/default.edc

CLEANFILES += \
data/themes/default.edj

data/themes/default.edj: Makefile data/themes/default.edc $(THEME_IMGS)
	$(MKDIR_P) $(@D)
	$(AM_V_EDJ)$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/default.edc \
	$(top_builddir)/data/themes/default.edj

clean-local:
	rm -f *.edj
