cmake_minimum_required(VERSION 3.20)
project(VideoSnapPro VERSION 2.0.0 LANGUAGES CXX)

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
)

qt_add_executable(VideoSnapPro ${SOURCES} ${HEADERS})

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
