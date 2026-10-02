cmake_minimum_required(VERSION 3.10)

project(emulationstation-all)

# Get rid of this pollution file
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

#-------------------------------------------------------------------------------
#set version number
if (DEFINED ENV{RECALBOX_VERSION})
    set(PROGRAM_VERSION_STRING $ENV{RECALBOX_VERSION})
elseif (DEFINED ENV{RECALBOX_VERSION_LABEL})
    set(PROGRAM_VERSION_STRING $ENV{RECALBOX_VERSION_LABEL})
else()
    set(PROGRAM_VERSION_STRING "10-dev")
endif()

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/es-app/src/EmulationStation.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/es-app/src/EmulationStation.h"
)

OPTION(FORCE_OPENGL     "Force Desktop OpenGL, ignore OpenGLES entirely"           OFF)
OPTION(FORCE_OPENGLES   "Force OpenGL ES 2.0, skip ES 3.1 auto-detection"         OFF)
OPTION(FORCE_OPENGLES31 "Force OpenGL ES 3.1 (fatal error if headers not found)"  OFF)
# Vulkan is EXPERIMENTAL and never auto-selected: it is picked only when this
# option is explicitly turned on. Auto-detection must keep resolving to GLES/GL
# so that no target silently switches renderer.
OPTION(FORCE_VULKAN     "Force the experimental Vulkan renderer (never auto-selected)" OFF)
OPTION(USE_LIBUNWIND "Use libunwind for DeathTrap stack walking (alpha/patron only)" OFF)

#-------------------------------------------------------------------------------
#add local find scripts to CMAKE path
LIST(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Packages
)

#-------------------------------------------------------------------------------
#set up OpenGL system variable
set(GLSystem "Desktop OpenGL" CACHE STRING "The OpenGL system to be used")
set_property(CACHE GLSystem PROPERTY STRINGS "Desktop OpenGL" "OpenGL ES" "OpenGL ES 3.1" "Vulkan")

#-------------------------------------------------------------------------------
# finding GL compatible library
# Detection priority (when no FORCE_* is set): GLES 3.1 > GLES 2.0 > Desktop GL
MESSAGE("Looking for OpenGL or OpenGLES")
find_package(PkgConfig REQUIRED)
if(NOT FORCE_OPENGLES AND NOT FORCE_OPENGLES31)
    pkg_check_modules(OPENGL gl)
endif()
#pkg_check_modules(EGL egl)
if(NOT FORCE_OPENGL)
    pkg_check_modules(OPENGLES glesv2)
endif()

# IMPORTANT: GLES is intentionally tested BEFORE Desktop GL below.
# On ARM/RPi, EmulationStation always boots on raw DRM/KMS (no X11/Wayland).
# GLES gets a context directly on DRM/KMS; Desktop GL (USE_OPENGL_CORE)
# imperatively needs X11 or Wayland — which ES never runs under. Since both
# gl.pc and glesv2.pc are present in the Pi sysroot, this ordering is the only
# thing keeping ES on GLES. Do NOT move the elseif(OPENGL_FOUND) branch first:
# it would silently switch ES to Core on Pi and break the DRM/KMS boot.
# Desktop GL Core is a last-resort fallback (glesv2 absent) or an explicit FORCE_OPENGL.
if(FORCE_VULKAN)
    # Explicitly requested: skip GL resolution entirely. Never reached otherwise.
    find_package(Vulkan REQUIRED)
    set(GLSystem "Vulkan")
    MESSAGE("Vulkan forced: lib=${Vulkan_LIBRARIES}  inc=${Vulkan_INCLUDE_DIRS}")
elseif(OPENGLES_FOUND AND NOT FORCE_OPENGL)
    if(NOT FORCE_OPENGLES)
        # Try GLES 3.1 first (optional — no REQUIRED so a missing header silently falls back)
        find_package(OpenGLES31)
    endif()
    if(OPENGLES31_FOUND AND NOT FORCE_OPENGLES)
        MESSAGE("OpenGLES 3.1 found: lib=${OPENGLES31_gl_LIBRARY}  inc=${OPENGLES31_INCLUDE_DIR}")
        set(GLSystem "OpenGL ES 3.1")
    else()
        find_package(OpenGLES2 REQUIRED)
        MESSAGE("OpenGLES 2.0 found: lib=${OPENGLES2_gl_LIBRARY}  inc=${OPENGLES2_INCLUDE_DIR}")
        set(GLSystem "OpenGL ES")
    endif()
elseif(OPENGL_FOUND)
    MESSAGE("OpenGL found ${OPENGL_LIBRARIES}")
    find_package(OpenGL REQUIRED)
else()
    MESSAGE(FATAL_ERROR "OpenGL or OpenGLES required!")
endif()

# Honour explicit FORCE_OPENGLES31: error out if we resolved to something else
if(FORCE_OPENGLES31 AND NOT ${GLSystem} MATCHES "3.1")
    MESSAGE(FATAL_ERROR "FORCE_OPENGLES31 is set but OpenGL ES 3.1 headers (GLES3/gl31.h) were not found.")
endif()

pkg_check_modules(BCMHOST bcm_host)

if(USE_LIBUNWIND)
    pkg_check_modules(LIBUNWIND REQUIRED libunwind)
    add_definitions(-DUSE_LIBUNWIND=1)
    message(STATUS "libunwind enabled for DeathTrap: ${LIBUNWIND_LIBRARIES}")
endif()

#-------------------------------------------------------------------------------
#set(MuPDF_USE_STATIC FALSE)
# finding necessary packages
find_package(Freetype REQUIRED)
find_package(FreeImage REQUIRED)
find_package(SDL2MIXER REQUIRED)
find_package(SDL2 REQUIRED) 
find_package(CURL REQUIRED)
find_package(PulseAudio REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(Udev REQUIRED)
find_package(libproc2 REQUIRED)
find_package(MuPDF REQUIRED)

#-------------------------------------------------------------------------------
#set up compiler flags and excutable names
if(BCMHOST_FOUND)
    add_definitions(-D_RPI_)
endif()

#-------------------------------------------------------------------------------
option(USE_KMSDRM "Use DRM/KMS backend." OFF)

if (USE_KMSDRM)
    add_definitions("-DUSE_KMSDRM=1")
    execute_process(COMMAND pkg-config libdrm --cflags OUTPUT_VARIABLE LIBDRM_INCLUDE OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND pkg-config libdrm --libs OUTPUT_VARIABLE LIBDRM_LIBRARY OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

#-------------------------------------------------------------------------------

if(CMAKE_COMPILER_IS_GNUCXX)
    #check for G++ 4.7+
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
    if (G++_VERSION VERSION_LESS 4.7)
            message(SEND_ERROR "You need at least G++ 4.7 to compile EmulationStation!")
    endif()

    if(OPTION_RECALBOX_PRODUCTION_BUILD)
        set(RECALBOX_PRODUCTION_BUILD "-D_RECALBOX_PRODUCTION_BUILD_=1")
    endif()

    if(OPTION_RECALBOX_EASY_DEBUG)
        set(OPTION_RECALBOX_EASY_DEBUG "-DFREEZE_MQTT=1 -DSLOW_LOADING=1")
    endif()

    if(OPTION_RECALBOX_FREEZE_MQTT)
        set(OPTION_RECALBOX_EASY_DEBUG "${OPTION_RECALBOX_EASY_DEBUG} -DFREEZE_MQTT=1")
    endif()

    if(OPTION_RECALBOX_SIMULATE_RRGBD)
        set(OPTION_RECALBOX_EASY_DEBUG "${OPTION_RECALBOX_EASY_DEBUG} -DOPTION_RECALBOX_SIMULATE_RRGBD=1")
    endif()

    if(OPTION_RECALBOX_NO_AUDIO)
        set(OPTION_RECALBOX_EASY_DEBUG "${OPTION_RECALBOX_EASY_DEBUG} -DOPTION_RECALBOX_NO_AUDIO=1")
    endif()

    if(BETA)
        set(OPTION_RECALBOX_EASY_DEBUG "${OPTION_RECALBOX_EASY_DEBUG} -DBETA=1")
    endif()

    if(OPTION_RECALBOX_DEBUG_FILE_TREE)
        set(OPTION_RECALBOX_EASY_DEBUG "${OPTION_RECALBOX_EASY_DEBUG} -DDEBUG_FILE_TREE=1")
    endif()

    #set up compiler flags for GCC
	if (CMAKE_BUILD_TYPE MATCHES Debug)
        # -fsanitize=address -fsanitize-address-use-after-scope
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTION_RECALBOX_EASY_DEBUG} ${LIBDRM_INCLUDE} -DDEBUG -Wno-unknown-pragmas -Wno-attributes -Wswitch-enum -Wswitch -O0 -g -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections -DPAHO_MQTT_STATIC=1 -DHIGH_PERFORMANCE=1 -D_7ZIP_ST")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTION_RECALBOX_EASY_DEBUG} ${LIBDRM_INCLUDE} -fconcepts-diagnostics-depth=3 -static-libstdc++ -std=c++20 -DDEBUG -Wno-unknown-pragmas -Wno-attributes -Wswitch-enum -Wswitch -O0 -g -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections -D_7ZIP_ST") #support C++11 for std::, optimize
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -DDEBUG -Wswitch-enum -Wswitch -O0 -g -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RECALBOX_PRODUCTION_BUILD} ${LIBDRM_INCLUDE} -Wno-unknown-pragmas -Wno-attributes -O3 -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections -DPAHO_MQTT_STATIC=1 -DHIGH_PERFORMANCE=1 -D_7ZIP_ST")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${RECALBOX_PRODUCTION_BUILD} ${LIBDRM_INCLUDE} -Wno-unknown-pragmas -std=c++20 -Wno-attributes -O3 -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections -D_7ZIP_ST") #support C++11 for std::, optimize
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3 -Wall -Wextra -ffunction-sections -fdata-sections -Wl,--gc-sections")  #-s = strip binary
	endif()
endif()

if(OPTION_SANITIZERS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O0 -fno-omit-frame-pointer -g3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O0 -fno-omit-frame-pointer -g3")
endif()

# USE_OPENGL is set alongside the specific backend define, so that code shared
# by every GL backend (RendererGLCommon, the rendering/opengl helpers) can guard
# itself with a single condition instead of repeating the three variants — and
# so it disappears entirely from a Vulkan build.
if(${GLSystem} MATCHES "Vulkan")
    add_definitions(-DUSE_VULKAN)
elseif(${GLSystem} MATCHES "Desktop OpenGL")
    add_definitions(-DUSE_OPENGL_CORE -DUSE_OPENGL)
elseif(${GLSystem} MATCHES "OpenGL ES 3.1")
    add_definitions(-DUSE_OPENGL_ES31 -DUSE_OPENGL)
else()
    add_definitions(-DUSE_OPENGL_ES -DUSE_OPENGL)
endif()

#-------------------------------------------------------------------------------
#add include directories
set(COMMON_INCLUDE_DIRS
    # OSX seems to have a conflicting platform.h somewhere and fails to find
    # getHomePath, so let use our includes paths first.
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/7zip
    ${CMAKE_CURRENT_SOURCE_DIR}/external/pistache/include
    ${CMAKE_CURRENT_SOURCE_DIR}/es-app/src
    ${CMAKE_CURRENT_SOURCE_DIR}/es-core/src
    ${FREETYPE_INCLUDE_DIRS}
    ${FreeImage_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${PULSEAUDIO_INCLUDE_DIR}
    ${UDEV_INCLUDE_DIRS}
    ${LIBUNWIND_INCLUDE_DIRS}
    ${MuPDF_INCLUDE_DIR}
)

if(${GLSystem} MATCHES "Vulkan")
    LIST(APPEND COMMON_INCLUDE_DIRS
        ${Vulkan_INCLUDE_DIRS}
    )
elseif(${GLSystem} MATCHES "Desktop OpenGL")
    LIST(APPEND COMMON_INCLUDE_DIRS
        ${OPENGL_INCLUDE_DIR}
    )
elseif(${GLSystem} MATCHES "OpenGL ES 3.1")
    LIST(APPEND COMMON_INCLUDE_DIRS
        ${OPENGLES31_INCLUDE_DIR}
    )
else()
    LIST(APPEND COMMON_INCLUDE_DIRS
        ${OPENGLES2_INCLUDE_DIR}
    )
endif()

#-------------------------------------------------------------------------------
#define libraries and directories
#link_directories(${Boost_LIBRARY_DIRS})

set(COMMON_LIBRARIES
    #${Boost_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${FreeImage_LIBRARIES}
	${SDL2_LIBRARY}
	${SDLMIXER_LIBRARY}
    ${CURL_LIBRARIES}
    ${PULSEAUDIO_LIBRARY}
    ${LIBDRM_LIBRARY}
    ${LIBLZMA_LIBRARIES}
    ${UDEV_LIBRARIES}
    ${LIBUNWIND_LIBRARIES}
    ${MuPDF_LIBRARIES}
    jpeg
    pugixml
    nanosvg
    liboping
    pistache
    resolv
    blkid
    lzma
    # MuPDF dependencies
    z
    openjp2
    harfbuzz
    jbig2dec
    gumbo
)

if(RECALBOX_LINK_MUJS)
    LIST(APPEND COMMON_LIBRARIES
            mujs
    )
endif()

if(OPTION_SANITIZERS)
    LIST(APPEND COMMON_LIBRARIES
            asan
    )
endif()

if(MSVC)
    LIST(APPEND COMMON_LIBRARIES
	winmm
    )
endif()
if(${GLSystem} MATCHES "Vulkan")
    LIST(APPEND COMMON_LIBRARIES
        ${Vulkan_LIBRARIES}
    )
elseif(${GLSystem} MATCHES "Desktop OpenGL")
    LIST(APPEND COMMON_LIBRARIES
        ${OPENGL_LIBRARIES}
    )
else()
    # Both GLES2 and GLES3.1 link against libGLESv2 (+ EGL).
    # On x86_64 Mesa, libGLESv2 is a thin shim over libGL — add GL explicitly
    # so the linker can resolve GL2+ symbols without a "DSO missing" error.
    if(${GLSystem} MATCHES "OpenGL ES 3.1")
        LIST(APPEND COMMON_LIBRARIES ${OPENGLES31_LIBRARIES})
    else()
        LIST(APPEND COMMON_LIBRARIES ${OPENGLES2_LIBRARIES})
    endif()
    find_library(LIBGL_LIBRARY GL)
    if(LIBGL_LIBRARY)
        LIST(APPEND COMMON_LIBRARIES GL)
    endif()
endif()

#-------------------------------------------------------------------------------
# set up build directories
set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

#-------------------------------------------------------------------------------
# add each component
execute_process (
        COMMAND bash -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/data/ && ./generate.sh"
)

add_subdirectory("external")
add_subdirectory("es-core")
add_subdirectory("es-app")


# i18n
find_program (MSGFMT_EXECUTABLE msgfmt)
find_program (MSGMERGE_EXECUTABLE msgmerge)
find_program (XGETTEXT_EXECUTABLE xgettext)
if(MSGFMT_EXECUTABLE AND MSGMERGE_EXECUTABLE AND XGETTEXT_EXECUTABLE)
	message (STATUS "Native language support enabled.")
	add_subdirectory (locale)
endif()

# Tests
add_subdirectory(tests)

# md5zip
add_subdirectory(tool-md5zip)

# Web micro-server
add_subdirectory(web-server)



message(
        "=========================================================================================================\n"
        "\n"
        "C    Flags  : ${CMAKE_C_FLAGS}\n"
        "C++  Flags  : ${CMAKE_CXX_FLAGS}\n"
        "Link Flags  : ${CMAKE_EXE_LINKER_FLAGS}\n"
        "GL System   : ${GLSystem}\n"
        "GL Include  : ${OPENGL_INCLUDE_DIR}${OPENGLES31_INCLUDE_DIR}${OPENGLES2_INCLUDE_DIR}\n"
        "GL Library  : ${OPENGL_LIBRARIES}${OPENGLES31_LIBRARIES}${OPENGLES2_LIBRARIES}\n"
        "SDL Include : ${SDL2_INCLUDE_DIR}\n"
        "SDL Library : ${SDL2_LIBRARY}\n"
        "\n"
        "=========================================================================================================\n"
)
