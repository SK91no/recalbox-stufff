# Googletest framework
file(GLOB GOOGLETEST_SOURCES googletest/src/gtest-all.cc)

# Test code
file(GLOB_RECURSE TESTS_SOURCES tests/*.cpp)

# Tested code & dependencies
file(GLOB_RECURSE TESTED_PATH
        ../es-app/src/games/classifications/*.cpp
        ../es-core/src/utils/*.cpp
        ../es-core/src/RootFolders.cpp
        ../external/7zip/*.c
        ../es-app/src/guis/menus/MenuDeserializer.cpp
        ../es-app/src/guis/menus/MenuItemType.cpp
        ../es-app/src/guis/menus/MenuDefinition.cpp
        ../external/pugixml/src/pugixml.cpp
)
# All tested code
set(ALL_TESTED_SOURCES ${TESTED_PATH})

find_package(SDL2 REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(Udev REQUIRED)
find_package(libproc2 REQUIRED)

include_directories(
        ${SDL2_INCLUDE_DIR}
        ../es-core/src
        ../es-app/src
        ../external
        ../external/pugixml
        googletest/include
        googletest
)

add_executable(
        emulationstation_test
        main.cpp
        ${GOOGLETEST_SOURCES}
        ${TESTS_SOURCES}
        ${ALL_TESTED_SOURCES}
)

target_compile_definitions(emulationstation_test PUBLIC TEST_ONLY=1)
target_link_libraries(emulationstation_test pthread ${SDL2_LIBRARY} curl zip resolv ${UDEV_LIBRARIES} ${LIBLZMA_LIBRARIES} procps::libproc2)
