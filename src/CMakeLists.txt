########### next target ###############

set(kredentials_SRCS main.cpp kredentials.cpp krb5_wrap.cpp resources.qrc)

qt5_add_resources(kredentials_SRCS resources.qrc)

set(CMAKE_AUTORCC ${AUTOGEN}) #resources

add_executable(kredentials ${kredentials_SRCS})

#target_link_libraries(kredentials ${QT_QTGUI_LIBRARY} QtGui)
#target_link_libraries(kredentials ${KDE5_KDECORE_LIBS} kdeui)
#target_link_libraries(kredentials ${KRB5_LIBRARIES} krb5)
target_link_libraries(kredentials
    Qt5::Widgets
    KF5::CoreAddons
    KF5::WidgetsAddons
    KF5::ConfigCore
    KF5::ConfigGui
    KF5::DBusAddons
    ${KRB5_LIBRARIES}
)

########### install files ###############
install(TARGETS kredentials ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES  kredentials.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
