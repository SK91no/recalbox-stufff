# FindOpenGLES2.cmake
# --------------------
# Find OpenGL ES 2.0 headers and library (libGLESv2).
#
# This module intentionally targets GLES 2.0 only and will NOT match the
# GLES 1.x (GLESv1_CM / GLES_CM) libraries that FindOpenGLES.cmake picks up.
#
# Imported result variables
# -------------------------
#  OPENGLES2_FOUND        - TRUE if GLES2 header and library were located
#  OPENGLES2_INCLUDE_DIR  - Directory containing GLES2/gl2.h
#  OPENGLES2_LIBRARIES    - Libraries to link against (GLESv2, + EGL when needed)
#
# Typical usage
# -------------
#   find_package(OpenGLES2 REQUIRED)
#   target_link_libraries(myTarget ${OPENGLES2_LIBRARIES})
#   target_include_directories(myTarget PRIVATE ${OPENGLES2_INCLUDE_DIR})

# -- Vendor blob stacks: ask pkg-config first -------------------------------
# On ARM Mali (and other vendor blob stacks), libGLESv2.so / libEGL.so are empty
# ELF stubs: they export no GL symbol at all and merely pull libmali.so.1 in as
# a NEEDED dependency. Linking against those stubs alone makes ld resolve
# glXxx() from libmali -- a DSO that is not on the command line -- and the link
# dies with "undefined reference to symbol 'glGetProgramInfoLog' /
# libmali.so.1: error adding symbols: DSO missing from command line".
# The glesv2.pc / egl.pc files shipped by those drivers do list the real
# library (-lmali) next to the stub, so trust pkg-config when it answers and
# keep the find_library() result as a fallback for stacks without .pc files.
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GLESV2 QUIET glesv2)
  pkg_check_modules(PC_EGL    QUIET egl)
endif()

# -- Header ----------------------------------------------------------------
find_path(OPENGLES2_INCLUDE_DIR
  NAMES GLES2/gl2.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/vc/include          # Raspberry Pi firmware / VideoCore IV
    /opt/graphics/OpenGL/include
    /usr/openwin/share/include
    /usr/X11R6/include
)

# -- Library ---------------------------------------------------------------
find_library(OPENGLES2_gl_LIBRARY
  NAMES GLESv2
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/vc/lib              # Raspberry Pi firmware / VideoCore IV
    /opt/graphics/OpenGL/lib
    /usr/openwin/lib
    /usr/shlib
    /usr/X11R6/lib
    /usr/lib/aarch64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /usr/lib/x86_64-linux-gnu
)

# -- EGL (optional companion, needed on most embedded targets) -------------
find_library(OPENGLES2_egl_LIBRARY
  NAMES EGL
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/vc/lib
    /opt/graphics/OpenGL/lib
    /usr/lib/aarch64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /usr/lib/x86_64-linux-gnu
)

# -- Compose result --------------------------------------------------------
set(OPENGLES2_FOUND FALSE)
if(OPENGLES2_gl_LIBRARY AND OPENGLES2_INCLUDE_DIR)
  # pkg-config link lines win over the bare stub path: they carry the vendor
  # library (-lmali on Mali) that actually holds the GL entry points.
  if(PC_GLESV2_FOUND)
    set(OPENGLES2_LIBRARIES ${PC_GLESV2_LDFLAGS})
  else()
    set(OPENGLES2_LIBRARIES ${OPENGLES2_gl_LIBRARY})
  endif()
  if(PC_EGL_FOUND)
    list(APPEND OPENGLES2_LIBRARIES ${PC_EGL_LDFLAGS})
  elseif(OPENGLES2_egl_LIBRARY)
    list(APPEND OPENGLES2_LIBRARIES ${OPENGLES2_egl_LIBRARY})
  endif()
  list(REMOVE_DUPLICATES OPENGLES2_LIBRARIES)
  set(OPENGLES2_FOUND TRUE)
endif()

# -- Standard CMake boilerplate --------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES2
  REQUIRED_VARS OPENGLES2_gl_LIBRARY OPENGLES2_INCLUDE_DIR
  FAIL_MESSAGE  "OpenGL ES 2.0 (libGLESv2 + GLES2/gl2.h) not found. Install libgles2-mesa-dev (Debian/Ubuntu) or equivalent."
)

mark_as_advanced(
  OPENGLES2_INCLUDE_DIR
  OPENGLES2_gl_LIBRARY
  OPENGLES2_egl_LIBRARY
)
