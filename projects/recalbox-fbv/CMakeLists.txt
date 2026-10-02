cmake_minimum_required(VERSION 2.8)
project(fbv2 C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

include_directories(.)

add_executable(fbv2
        bmp.c
        fb_display.c
        fbv.h
        gif.c
        jpeg.c
        main.c
        png.c
        transforms.c)

target_link_libraries(fbv2 gif png jpeg)
