cmake_minimum_required(VERSION 2.8)

project(KOBOREDUX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 7)
set(VERSION_PATCH 2)
set(VERSION_BUILD 0)

option(DEMO_BUILD "Build Kobo Redux demo." OFF)

if(DEMO_BUILD)
	# Demo build differcences:
	#	- Append "-demo" to the package/install dir/... name.
	#	- Exclude the mono and chip themes.
	#	- Install the Kobo Redux Demo themes.
	set(KOBO_PACKAGE_NAME "koboredux-demo")
	set(DEMO_EXCLUDE_GFX PATTERN mono EXCLUDE)
	set(DEMO_EXCLUDE_SFX PATTERN chip EXCLUDE)
	set(EXT_GFX_DIR "demo-data/gfx")
	set(EXT_SFX_DIR "demo-data/sfx")
else(DEMO_BUILD)
	set(KOBO_PACKAGE_NAME "koboredux")
	set(DEMO_EXCLUDE_GFX "")
	set(DEMO_EXCLUDE_SFX "")
	set(EXT_GFX_DIR "full-data/gfx")
	set(EXT_SFX_DIR "full-data/sfx")
endif(DEMO_BUILD)

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(PACKAGE "${KOBO_PACKAGE_NAME}-${VERSION_STRING}")

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

install(DIRECTORY data/gfx ${EXT_GFX_DIR}
	DESTINATION "share/${KOBO_PACKAGE_NAME}"
	FILES_MATCHING
	${DEMO_EXCLUDE_GFX}
	PATTERN "*.png"
	PATTERN "*.gpl"
	PATTERN "*.theme")
install(DIRECTORY data/sfx  ${EXT_SFX_DIR}
	DESTINATION "share/${KOBO_PACKAGE_NAME}"
	FILES_MATCHING
	${DEMO_EXCLUDE_SFX}
	PATTERN "*.a2s")

#TODO: Score directory?

# CPack general
set(CPACK_GENERATOR "DEB" "RPM" "TBZ2")
set(CPACK_SET_DESTDIR ON)
set(CPACK_PACKAGE_RELOCATABLE "false")
set(CPACK_PACKAGE_NAME "${KOBO_PACKAGE_NAME}")
set(CPACK_PACKAGE_VERSION "${VERSION_STRING}")
set(CPACK_PACKAGE_CONTACT "David Olofson <david@olofson.net>")
set(CPACK_RESOURCE_FILE_LICENSE "${KOBOREDUX_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${KOBOREDUX_SOURCE_DIR}/README.md")

# CPack DEB
set(CPACK_DEBIAN_PACKAGE_SECTION "Games")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2,libsdl2-image")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION_SUMMARY "An arcade style 2D shooter")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Kobo Redux is a frantic 80's style 2D shooter, inspired by the look and feel of 90's arcade cabinets. The gameplay is fast and unforgiving, although with less of the frustrating quirkiness of the actual games of the 80's. A true challenge in the spirit of the arcade era!")

# CPack PRM
set(CPACK_RPM_PACKAGE_GROUP "Games")
set(CPACK_RPM_PACKAGE_SUMMARY "${CPACK_DEBIAN_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_DEBIAN_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_LICENSE "zlib")
set(CPACK_RPM_PACKAGE_REQUIRES "libSDL2-2.0.so.0,libSDL2_image-2.0.so.0,libm.so.6")

include(CPack)
