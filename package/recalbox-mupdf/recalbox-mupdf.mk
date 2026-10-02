################################################################################
#
# recalbox-mupdf
#
################################################################################

RECALBOX_MUPDF_VERSION = 1.27.2
RECALBOX_MUPDF_SOURCE = mupdf-$(RECALBOX_MUPDF_VERSION)-source.tar.lz
RECALBOX_MUPDF_SITE = https://mupdf.com/downloads/archive
RECALBOX_MUPDF_LICENSE = AGPL-3.0+
RECALBOX_MUPDF_LICENSE_FILES = COPYING
RECALBOX_MUPDF_CPE_ID_VENDOR = artifex
RECALBOX_MUPDF_INSTALL_STAGING = YES
RECALBOX_MUPDF_DEPENDENCIES = \
	freetype \
	gumbo-parser \
	harfbuzz \
	host-pkgconf \
	jbig2dec \
	lcms2 openjpeg \
	zlib brotli

# mupdf doesn't use CFLAGS and LIBS but XCFLAGS and XLIBS instead.
# with USE_SYSTEM_LIBS it will try to use system libraries instead of the bundled ones.
# EXCEPTION: USE_SYSTEM_LIBJPEG=no forces mupdf to compile and statically link its
# bundled libjpeg (thirdparty/libjpeg). This avoids ABI mismatches with the system
# libjpeg variant that may be present at runtime (e.g. another package on the target
# pulling IJG libjpeg v9, JPEG_LIB_VERSION=90, while mupdf was built against the
# turbo SONAME 8 headers with JPEG_LIB_VERSION=80, manifests as black PDF pages on
# any content containing JPEG images).
RECALBOX_MUPDF_MAKE_ENV = $(TARGET_MAKE_ENV) $(TARGET_CONFIGURE_OPTS) \
	XCFLAGS="$(TARGET_CFLAGS)" \
	XLIBS="$(TARGET_LDFLAGS)" \
	USE_SYSTEM_LIBS=yes \
	USE_SYSTEM_LIBJPEG=no

RECALBOX_MUPDF_MAKE_OPTS = \
	HAVE_OBJCOPY=no \
	USE_SYSTEM_LIBJPEG=no \
	prefix="/usr"

ifeq ($(BR2_STATIC_LIBS),y)
RECALBOX_MUPDF_MAKE_OPTS += shared=no
else
RECALBOX_MUPDF_MAKE_OPTS += shared=yes
endif

ifeq ($(BR2_PACKAGE_XLIB_LIBX11)$(BR2_PACKAGE_XLIB_LIBXEXT),yy)
RECALBOX_MUPDF_MAKE_OPTS += HAVE_X11=yes
RECALBOX_MUPDF_DEPENDENCIES += xlib_libX11 xlib_libXext
else
RECALBOX_MUPDF_MAKE_OPTS += HAVE_X11=no
endif

ifeq ($(BR2_PACKAGE_LIBFREEGLUT),y)
RECALBOX_MUPDF_DEPENDENCIES += libfreeglut
else
RECALBOX_MUPDF_MAKE_OPTS += HAVE_GLUT=no
endif

define RECALBOX_MUPDF_BUILD_CMDS
	$(RECALBOX_MUPDF_MAKE_ENV) $(MAKE) -C $(@D) $(RECALBOX_MUPDF_MAKE_OPTS) all
endef

define RECALBOX_MUPDF_INSTALL_STAGING_CMDS
	$(RECALBOX_MUPDF_MAKE_ENV) $(MAKE) -C $(@D) $(RECALBOX_MUPDF_MAKE_OPTS) \
		DESTDIR="$(STAGING_DIR)" install-libs
endef

define RECALBOX_MUPDF_INSTALL_TARGET_CMDS
	$(RECALBOX_MUPDF_MAKE_ENV) $(MAKE) -C $(@D) $(RECALBOX_MUPDF_MAKE_OPTS) \
		DESTDIR="$(TARGET_DIR)" install
endef

$(eval $(generic-package))
