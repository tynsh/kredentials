# Some build reminders:
# Run cmake with "-D CMAKE_BUILD_TYPE=Debug" to generate a debug build
# Run make with "VERBOSE=1" to see the compiler command line

set(KREDENTIALS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${KREDENTIALS_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include_directories(${KRB5_INCLUDE_DIRS})
set(LIBS ${LIBS} ${KRB5_LIBRARIES})

find_package(KDE4 REQUIRED)
find_package(Kerberos5 REQUIRED)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=9876)

include(KDE4Defaults)

include(MacroLibrary)

add_subdirectory(src)
add_subdirectory(cmake)
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(TOPSUBDIRS)")
