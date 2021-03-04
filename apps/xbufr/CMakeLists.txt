set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG true)

find_package(Qt6Widgets QUIET)
if(Qt6_FOUND)
  find_package(Qt6Core REQUIRED)
  find_package(Qt6Gui REQUIRED)
  find_package(Qt6Widgets REQUIRED)
  set(_qt_target_libs Qt6::Core Qt6::Gui Qt6::Widgets)
else()
  find_package(Qt5 COMPONENTS Widgets REQUIRED)
  if(Qt5_FOUND)
    find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)
    set(_qt_target_libs Qt5::Core Qt5::Gui Qt5::Widgets)
  else()
    message(FATAL "Missing Qt")
  endif()
endif()

add_compile_definitions(QT_NO_KEYWORDS)
add_compile_definitions(QT_WARN_ON)
remove_definitions(QT_CONSOLE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(UIS
    mainwindow.ui
)

if(Qt6_FOUND)
  QT6_WRAP_UI( UI_HEADERS ${UIS} )
else()
  QT5_WRAP_UI( UI_HEADERS ${UIS} )
endif()

if(APPLE)
  set(MACOSX_BUNDLE_ICON_FILE bufr.icns)
  set(XBUFR_MACOS_ICON images/bufr.icns)
  set_source_files_properties(${XBUFR_MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  set(XBUFR_MACOS MACOSX_BUNDLE)
endif()

if(WIN32)
  set(XBUFR_WIN32_ICON xbufr.rc)
  set(XBUFR_WIN32 WIN32)
endif()

add_executable(xbufr
    ${XBUFR_MACOS} ${XBUFR_MACOS_ICON}
    ${XBUFR_WIN32} ${XBUFR_WIN32_ICON}
    main.cpp
    myapplication.cpp
    mainwindow.cpp
    nodemodel.cpp
    versiondialog.cpp
    valuesmodel.cpp
    xbufr.qrc
    ${UI_HEADERS}
)

if(APPLE)
  set_target_properties(xbufr PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
endif()

target_link_libraries(xbufr PRIVATE
    dbufr
    ${_qt_target_libs}
)

install(
    TARGETS xbufr
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin)

if(APPLE)
install(
    CODE "execute_process(COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/create_dmg.sh\"
                                  \"${CMAKE_INSTALL_PREFIX}\"
                          WORKING_DIRECTORY \"${CMAKE_PROJECT_BINARY_DIR}\"
                         )"
)
endif()
