cmake_minimum_required(VERSION 3.16)

project(GeoPhotoMap VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets WebEngineWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WebEngineWidgets)
find_package(Exiv2 CONFIG QUIET)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GeoPhotoMap
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

else()
    if(ANDROID)
        add_library(GeoPhotoMap SHARED
            ${PROJECT_SOURCES}
        )

    else()
        add_executable(GeoPhotoMap
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(GeoPhotoMap PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::WebEngineWidgets)

if(Exiv2_FOUND)
    if(TARGET Exiv2::exiv2)
        target_link_libraries(GeoPhotoMap PRIVATE Exiv2::exiv2)
    elseif(TARGET exiv2lib)
        target_link_libraries(GeoPhotoMap PRIVATE exiv2lib)
    elseif(EXIV2_LIBRARIES)
        target_link_libraries(GeoPhotoMap PRIVATE ${EXIV2_LIBRARIES})
    endif()
    target_compile_definitions(GeoPhotoMap PRIVATE HAVE_EXIV2)
    if(EXIV2_INCLUDE_DIRS)
        target_include_directories(GeoPhotoMap PRIVATE ${EXIV2_INCLUDE_DIRS})
    endif()
else()
    message(WARNING "Exiv2 not found. EXIF GPS write will be disabled.")
endif()


if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.GeoPhotoMap)
endif()
set_target_properties(GeoPhotoMap PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS GeoPhotoMap
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GeoPhotoMap)
endif()
