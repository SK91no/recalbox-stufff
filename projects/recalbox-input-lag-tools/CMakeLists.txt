cmake_minimum_required(VERSION 3.16)
project(controller_latency VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod>=2.0)

add_executable(controller_latency controller_latency.c)

target_include_directories(controller_latency PRIVATE ${GPIOD_INCLUDE_DIRS})
target_link_libraries(controller_latency PRIVATE ${GPIOD_LIBRARIES} m)
target_link_directories(controller_latency PRIVATE ${GPIOD_LIBRARY_DIRS})
target_compile_options(controller_latency PRIVATE -Wall -Wextra ${GPIOD_CFLAGS_OTHER})

install(TARGETS controller_latency DESTINATION bin)