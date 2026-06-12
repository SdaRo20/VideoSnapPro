cmake_minimum_required(VERSION 3.20)
project(VideoSnapPro VERSION 2.1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/downloadworker.cpp
    src/downloadqueue.cpp
    src/settingsdialog.cpp
    src/historydialog.cpp
    src/metafetcher.cpp
    src/thememanager.cpp
    src/converterdialog.cpp
    src/autoupdater.cpp
    src/updatedialog.cpp
    src/splashscreen.cpp
    src/scraperengine.cpp
    src/scraperdialog.cpp
)

set(HEADERS
    include/mainwindow.h
    include/downloadworker.h
    include/downloadqueue.h
    include/settingsdialog.h
    include/historydialog.h
    include/metafetcher.h
    include/thememanager.h
    include/converterdialog.h
    include/downloaditem.h
    include/progressdelegate.h
    include/platformdetector.h
    include/autoupdater.h
    include/updatedialog.h
    include/splashscreen.h
    include/scraperengine.h
    include/scraperdialog.h
)

set(APP_RESOURCES resources/resources.qrc)

# Windows: embed .ico into .exe using absolute path
if(WIN32)
    # Generate app.rc with absolute path so windres can find the icon
    set(ICO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
         "IDI_ICON1 ICON \"${ICO_PATH}\"\n")
    set(APP_RESOURCES ${APP_RESOURCES} "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

qt_add_executable(VideoSnapPro ${SOURCES} ${HEADERS} ${APP_RESOURCES})

target_link_libraries(VideoSnapPro PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

if(WIN32)
    set_target_properties(VideoSnapPro PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
if(APPLE)
    set_target_properties(VideoSnapPro PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER com.videosnapro.app
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    )
endif()
