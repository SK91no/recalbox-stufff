# FindOpenGLES31.cmake
# --------------------
# Find OpenGL ES 3.1 headers and library.
#
# There is no such thing as a "libGLESv3": every driver stack (Mesa, Mali,
# Adreno, Videocore VI...) ships the whole ES 3.x entry point set inside
# libGLESv2, and advertises the level through the GLES3/gl3*.h headers only.
# So this module keys the *version* off <GLES3/gl31.h> — the exact header
# es-core/src/rendering/opengl/GLHeaders.h includes for USE_OPENGL_ES31 — and
# keys the *library* off GLESv2, exactly like FindOpenGLES2.cmake does.
#
# Consequence: the header is the whole detection. A stack whose headers stop at
# GLES2/gl2.h (Raspberry Pi VideoCore IV and its /opt/vc blobs, for instance)
# simply won't be found here, and the caller falls back to GLES 2.0.
#
# Imported result variables
# -------------------------
#  OPENGLES31_FOUND        - TRUE if GLES 3.1 header and library were located
#  OPENGLES31_INCLUDE_DIR  - Directory containing GLES3/gl31.h
#  OPENGLES31_LIBRARIES    - Libraries to link against (GLESv2, + EGL when needed)
#
# Typical usage
# -------------
#   find_package(OpenGLES31)
#   target_link_libraries(myTarget ${OPENGLES31_LIBRARIES})
#   target_include_directories(myTarget PRIVATE ${OPENGLES31_INCLUDE_DIR})

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
# glesv3.pc is tried first for the rare stacks that ship one; Mesa and Mali
# both answer on glesv2.pc only.
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GLESV31 QUIET glesv3)
  if(NOT PC_GLESV31_FOUND)
    pkg_check_modules(PC_GLESV31 QUIET glesv2)
  endif()
  pkg_check_modules(PC_EGL31 QUIET egl)
endif()

# -- Header ----------------------------------------------------------------
# /opt/vc is deliberately absent: the Raspberry Pi firmware blobs top out at
# GLES 2.0, so probing them for a GLES3 header can only ever waste time.
find_path(OPENGLES31_INCLUDE_DIR
  NAMES GLES3/gl31.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/graphics/OpenGL/include
    /usr/openwin/share/include
    /usr/X11R6/include
)

# -- Library ---------------------------------------------------------------
find_library(OPENGLES31_gl_LIBRARY
  NAMES GLESv2
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/graphics/OpenGL/lib
    /usr/openwin/lib
    /usr/shlib
    /usr/X11R6/lib
    /usr/lib/aarch64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /usr/lib/x86_64-linux-gnu
)

# -- EGL (optional companion, needed on most embedded targets) -------------
find_library(OPENGLES31_egl_LIBRARY
  NAMES EGL
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/graphics/OpenGL/lib
    /usr/lib/aarch64-linux-gnu
    /usr/lib/arm-linux-gnueabihf
    /usr/lib/x86_64-linux-gnu
)

# -- Compose result --------------------------------------------------------
set(OPENGLES31_FOUND FALSE)
if(OPENGLES31_gl_LIBRARY AND OPENGLES31_INCLUDE_DIR)
  # pkg-config link lines win over the bare stub path: they carry the vendor
  # library (-lmali on Mali) that actually holds the GL entry points.
  if(PC_GLESV31_FOUND)
    set(OPENGLES31_LIBRARIES ${PC_GLESV31_LDFLAGS})
  else()
    set(OPENGLES31_LIBRARIES ${OPENGLES31_gl_LIBRARY})
  endif()
  if(PC_EGL31_FOUND)
    list(APPEND OPENGLES31_LIBRARIES ${PC_EGL31_LDFLAGS})
  elseif(OPENGLES31_egl_LIBRARY)
    list(APPEND OPENGLES31_LIBRARIES ${OPENGLES31_egl_LIBRARY})
  endif()
  list(REMOVE_DUPLICATES OPENGLES31_LIBRARIES)
  set(OPENGLES31_FOUND TRUE)
endif()

# -- Standard CMake boilerplate --------------------------------------------
# QUIET by default: the caller probes GLES 3.1 without REQUIRED and falls back
# to GLES 2.0, so a miss here is a normal outcome, not a problem to report.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES31
  REQUIRED_VARS OPENGLES31_gl_LIBRARY OPENGLES31_INCLUDE_DIR
  FAIL_MESSAGE  "OpenGL ES 3.1 (libGLESv2 + GLES3/gl31.h) not found. Install libgles2-mesa-dev (Debian/Ubuntu) or equivalent."
)

mark_as_advanced(
  OPENGLES31_INCLUDE_DIR
  OPENGLES31_gl_LIBRARY
  OPENGLES31_egl_LIBRARY
)
