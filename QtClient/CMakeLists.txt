cmake_minimum_required(VERSION 3.16)
project(LGA_OpenInNukeX VERSION 1.73.0 LANGUAGES CXX)

# Generate compile_commands.json for IntelliSense and clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS deployment settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")

    # AGL (Apple Graphics Library) es un framework legacy de OpenGL que Apple removio
    # en macOS 12+. Bloqueamos que FindOpenGL de CMake lo detecte o proponga.
    set(OPENGL_agl_LIBRARY "" CACHE STRING "AGL library" FORCE)
    set(OPENGL_AGL_INCLUDE_DIR "" CACHE PATH "AGL include dir" FORCE)
endif()

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

if(APPLE)
    # Qt6 usa el interface target WrapOpenGL::WrapOpenGL para OpenGL en macOS
    # (definido en $QTDIR/lib/cmake/Qt6/FindWrapOpenGL.cmake). Ese archivo agrega
    # "-framework AGL" incondicionalmente como fallback cuando find_library no
    # encuentra AGL (removido en macOS 12+). Lo purgamos aca para que ni el
    # sistema ni ningun consumer de Qt6::Widgets traiga AGL por transitividad.
    if(TARGET WrapOpenGL::WrapOpenGL)
        get_target_property(_wo_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
        if(_wo_libs)
            set(_wo_new_libs "")
            set(_wo_changed FALSE)
            foreach(_wo_lib ${_wo_libs})
                if("${_wo_lib}" MATCHES "(^|/)AGL(\\.framework)?$" OR "${_wo_lib}" MATCHES "-framework[ \t]+AGL")
                    set(_wo_changed TRUE)
                else()
                    list(APPEND _wo_new_libs "${_wo_lib}")
                endif()
            endforeach()
            if(_wo_changed)
                set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES INTERFACE_LINK_LIBRARIES "${_wo_new_libs}")
                message(STATUS "AGL framework removed from WrapOpenGL::WrapOpenGL")
            endif()
        endif()
    endif()
endif()

set(CMAKE_AUTOMOC ON)

# Source files
set(SOURCES
    src/main.cpp
    src/nukeopener.cpp
    src/nukeopener.h
    src/configwindow.cpp
    src/configwindow.h
    src/logger.cpp
    src/logger.h
    src/nukescanner.cpp
    src/nukescanner.h
    src/qflowlayout.cpp
    src/qflowlayout.h
)

add_executable(LGA_OpenInNukeX ${SOURCES})

target_link_libraries(LGA_OpenInNukeX
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

target_compile_definitions(LGA_OpenInNukeX PRIVATE
    OPENINNUKEX_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

# ── Windows ──────────────────────────────────────────────────────────────────
if(WIN32)
    set_target_properties(LGA_OpenInNukeX PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(LGA_OpenInNukeX PRIVATE LGA_OpenInNukeX.rc)
    enable_language(RC)
    target_link_libraries(LGA_OpenInNukeX shell32 dwmapi)
endif()

# ── macOS ─────────────────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(LGA_OpenInNukeX PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "LGA_OpenInNukeX"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_IDENTIFIER "com.lga.openinnukex"
        MACOSX_BUNDLE_ICON_FILE "LGA_OpenInNukeX"
    )

    # Embed icon into the bundle
    set(MACOSX_ICON "${CMAKE_SOURCE_DIR}/resources/LGA_OpenInNukeX.icns")
    target_sources(LGA_OpenInNukeX PRIVATE "${MACOSX_ICON}")
    set_source_files_properties("${MACOSX_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Embed dark_theme.qss into the bundle Resources
    set(QSS_FILE "${CMAKE_SOURCE_DIR}/dark_theme.qss")
    target_sources(LGA_OpenInNukeX PRIVATE "${QSS_FILE}")
    set_source_files_properties("${QSS_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS LGA_OpenInNukeX
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
