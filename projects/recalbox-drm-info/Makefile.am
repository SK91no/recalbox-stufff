AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = recalbox-drm-info
recalbox_drm_info_CFLAGS = $(AM_CFLAGS) $(LIBDRM_CFLAGS) $(XRANDR_CFLAGS) $(X11_CFLAGS)
recalbox_drm_info_LDADD = $(LDADD) $(LIBDRM_LIBS) $(XRANDR_LIBS) $(X11_LIBS)
recalbox_drm_info_SOURCES = recalbox-drm-info.c

clean-local:
	@rm config.status configure config.log
	@rm Makefile
	@rm -r autom4te.cache/
	@rm aclocal.m4
	@rm compile install-sh missing Makefile.in
