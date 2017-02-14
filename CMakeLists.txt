cmake_minimum_required(VERSION 2.8)

project(KOBOREDUX)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 7)
set(VERSION_PATCH 3)
set(VERSION_BUILD 0)

option(DEMO_BUILD "Build Kobo Redux demo." OFF)
option(WIN32_ZIP_BUILD "Build flat Win32 ZIP archive." OFF)
option(WORKDIR "Top level build/staging directory."
	"${KOBOREDUX_SOURCE_DIR}/build")
option(KOBO_EXTRA_LIBRARIES "Extra libraries to link with." "")

message(".-----------------------------------------")
message("|      DEMO_BUILD: ${DEMO_BUILD}")
message("| WIN32_ZIP_BUILD: ${WIN32_ZIP_BUILD}")
message("|         WORKDIR: ${WORKDIR}")
message("'-----------------------------------------")

if(DEMO_BUILD)
	# Demo build differcences:
	#	- Append "-demo" to the package/install dir/... name.
	#	- Exclude the mono and chip themes.
	#	- Install the Kobo Redux Demo themes.
	if(WIN32)
		set(KOBO_PACKAGE_NAME "KoboRedux-Demo")
	else(WIN32)
		set(KOBO_PACKAGE_NAME "koboredux-demo")
	endif(WIN32)
	set(DEMO_EXCLUDE_GFX PATTERN mono EXCLUDE)
	set(DEMO_EXCLUDE_SFX PATTERN chip EXCLUDE)
	set(EXT_DATA_DIR "${WORKDIR}/demo-data")
else(DEMO_BUILD)
	if(WIN32)
		set(KOBO_PACKAGE_NAME "KoboRedux")
	else(WIN32)
		set(KOBO_PACKAGE_NAME "koboredux")
	endif(WIN32)
	set(DEMO_EXCLUDE_GFX "")
	set(DEMO_EXCLUDE_SFX "")
	set(EXT_DATA_DIR "${WORKDIR}/full-data")
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

if(WIN32 AND WIN32_ZIP_BUILD)
	# Of course, you're not supposed to use 'make install' when using this!
	# This is a dirty hack to build Win32 ZIP archives.
	set(KOBO_BIN_DIR "/")
	set(KOBO_SHARE_DIR "/")
	set(KOBO_DOC_DIR "/")
else(WIN32 AND WIN32_ZIP_BUILD)
	set(KOBO_BIN_DIR "bin")
	set(KOBO_SHARE_DIR "share/${KOBO_PACKAGE_NAME}")
	set(KOBO_DOC_DIR "share/doc/${KOBO_PACKAGE_NAME}")
endif(WIN32 AND WIN32_ZIP_BUILD)

add_subdirectory(src)

# Game data files
install(DIRECTORY data/gfx "${EXT_DATA_DIR}/gfx"
	DESTINATION "${KOBO_SHARE_DIR}"
	FILES_MATCHING
	${DEMO_EXCLUDE_GFX}
	PATTERN "*.png"
	PATTERN "*.gpl"
	PATTERN "*.theme")
install(DIRECTORY data/sfx "${EXT_DATA_DIR}/sfx"
	DESTINATION "${KOBO_SHARE_DIR}"
	FILES_MATCHING
	${DEMO_EXCLUDE_SFX}
	PATTERN "*.a2s"
	PATTERN "*.theme")

# Documentation
file(GLOB doc_files
	"${KOBOREDUX_SOURCE_DIR}/*.md"
	"${KOBOREDUX_SOURCE_DIR}/LICENSE*")
list(REMOVE_ITEM doc_files "${KOBOREDUX_SOURCE_DIR}/INSTALL.md")
install(FILES ${doc_files}
	DESTINATION "${KOBO_DOC_DIR}")

# Desktop entry
if(NOT WIN32)
	set(KOBO_DE_FILE "net.olofson.${KOBO_PACKAGE_NAME}.desktop")
	install(FILES "${KOBOREDUX_SOURCE_DIR}/icons/koboredux.png"
		DESTINATION "share/pixmaps"
		RENAME "${KOBO_PACKAGE_NAME}.png")
	install(FILES "${KOBOREDUX_SOURCE_DIR}/icons/${KOBO_DE_FILE}"
		DESTINATION "share/applications")
endif(NOT WIN32)

# License agreement for proprietary data, if any
if(WIN32)
	install(FILES "${EXT_DATA_DIR}/EULA.txt"
		DESTINATION "${KOBO_DOC_DIR}"
		OPTIONAL)
else(WIN32)
	file(REMOVE "${PROJECT_BINARY_DIR}/copyright")
	if(EXISTS "${EXT_DATA_DIR}/EULA.txt")
		file(READ "${EXT_DATA_DIR}/EULA.txt" cpr)
	else()
		message("WARNING: No EULA found; using LICENSE.game_code.txt.")
		file(READ "${KOBOREDUX_SOURCE_DIR}/LICENSE.game_code.txt" cpr)
	endif()
	string(REPLACE "\r" "" cpr ${cpr})
	file(WRITE "${PROJECT_BINARY_DIR}/copyright" ${cpr})
	install(FILES "${PROJECT_BINARY_DIR}/copyright"
		DESTINATION "${KOBO_DOC_DIR}")
endif(WIN32)

# Win32 redistributables (SDL2, Audiality 2, libstdc++ etc)
if(WIN32)
	file(GLOB win32_redist_files "${WORKDIR}/win32-redist/*")
	install(FILES ${win32_redist_files}
		DESTINATION "${KOBO_BIN_DIR}")
endif(WIN32)

# CPack general
if(WIN32)
	# Windows ZIP archive
	set(CPACK_GENERATOR "ZIP")
else(WIN32)
	# Linux packages
	set(CPACK_GENERATOR "DEB" "RPM" "TBZ2")
endif(WIN32)
set(CPACK_SET_DESTDIR ON)
set(CPACK_PACKAGE_RELOCATABLE false)
set(CPACK_STRIP_FILES true)
set(CPACK_PACKAGE_NAME "${KOBO_PACKAGE_NAME}")
set(CPACK_PACKAGE_VERSION "${VERSION_STRING}")
set(CPACK_PACKAGE_CONTACT "David Olofson <david@olofson.net>")
if(EXISTS "${EXT_DATA_DIR}/EULA.txt")
	set(CPACK_RESOURCE_FILE_LICENSE "${EXT_DATA_DIR}/EULA.txt")
else()
	set(CPACK_RESOURCE_FILE_LICENSE
		"${KOBOREDUX_SOURCE_DIR}/LICENSE.game_code.txt")
endif()
set(CPACK_RESOURCE_FILE_README "${KOBOREDUX_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Arcade style 2D shooter")
set(CPACK_PACKAGE_DESCRIPTION "Frantic 80's style 2D shooter, inspired by the look and feel of 90's arcades.")

# CPack DEB
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SUMMARY "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Arcade style 2D shooter\n Kobo Redux is a frantic 80's style 2D shooter, inspired by the look and\n feel of 90's arcade cabinets. The game play is fast and unforgiving,\n although with less of the frustrating quirkiness of the actual games of\n the 80's. A true challenge in the spirit of the arcade era!")

# CPack PRM
set(CPACK_RPM_PACKAGE_GROUP "Games")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_DEBIAN_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
set(CPACK_RPM_PACKAGE_URL "http://koboredux.com")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
	/usr/share/applications /usr/share/pixmaps)

include(CPack)
