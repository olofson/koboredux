cmake_minimum_required(VERSION 2.8)

project(KOBOREDUX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 7)
set(VERSION_PATCH 1)
set(VERSION_BUILD 0)

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PACKAGE "koboredux-${VERSION_STRING}")

# Release build: full optimization, no debug features, no debug info
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Maintainer build: No optimizations, lots of warnings, fail on warnings
set(f "-O0 -g -Wall -Wwrite-strings -Wcast-align")
set(f "${f} -Wbad-function-cast -Waggregate-return")
set(f "${f} -Wstrict-prototypes -fno-builtin -Wshadow")
set(f "${f} -Wdeclaration-after-statement -Wmissing-prototypes")
set(f "${f} -Wmissing-declarations -Wdisabled-optimization")
set(CMAKE_C_FLAGS_MAINTAINER "${f} -Werror")

# Debug build: Maintainer + extra debug features, don't fail on warnings
set(f "${f} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG ${f})

add_subdirectory(src)

install(DIRECTORY data/gfx DESTINATION share/koboredux
	FILES_MATCHING
	PATTERN "*.png"
	PATTERN "*.gpl"
	PATTERN "*.theme")
install(DIRECTORY data/sfx DESTINATION share/koboredux
	FILES_MATCHING PATTERN "*.a2s")

#TODO: Score directory?
