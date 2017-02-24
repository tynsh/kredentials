# Some build reminders:
# Run cmake with "-D CMAKE_BUILD_TYPE=Debug" to generate a debug build
# Run make with "VERBOSE=1" to see the compiler command line
project (kredentials)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.2.0")

set(KREDENTIALS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/cmake
	${KREDENTIALS_SOURCE_DIR}/cmake/modules)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

add_subdirectory(src)
add_subdirectory(cmake)

include_directories(${KRB5_INCLUDE_DIRS})
set(LIBS ${LIBS} ${KRB5_LIBRARIES})
find_package(Kerberos5 REQUIRED)

# Find Qt modules
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS 
    Core    # QCommandLineParser, QStringLiteral
    Widgets # QApplication 
)

# Find KDE modules
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons      # KAboutData
    I18n            # KLocalizedString
    WidgetsAddons   # KMessageBox
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
    
message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(TOPSUBDIRS)")
