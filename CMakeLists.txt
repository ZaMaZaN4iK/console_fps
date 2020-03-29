cmake_minimum_required(VERSION 3.1)
project(console_game)
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# ncursesw settings
set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)

include(FindCurses)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

# sources
set(SOURCE_FILES main.cpp)

# exec target
add_executable(console_game ${SOURCE_FILES})

# link
target_link_libraries( console_game ${CURSES_LIBRARIES})

