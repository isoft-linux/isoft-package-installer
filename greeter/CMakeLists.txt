add_definitions(-std=c++11 -DTRANSLATION_DOMAIN=\"${GETTEXT_PACKAGE}\")

include_directories(
    ${QT_INCLUDES}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_AUTORCC ON)

set(isoftpackageinstaller_PART_SRCS
    main.cpp
    stepwidget.cpp
    welcomewidget.cpp
    packagewidget.cpp
    installwidget.cpp
)

qt5_add_dbus_interface(isoftpackageinstaller_PART_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/org.isoftlinux.Install.xml
    installgenerated
)

add_executable(isoft-package-installer ${isoftpackageinstaller_PART_SRCS})

target_link_libraries(isoft-package-installer
    Qt5::Gui
    Qt5::Widgets
    Qt5::DBus
    KF5::I18n
)

install(TARGETS isoft-package-installer ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/isoft-package-installer.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
