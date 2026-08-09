cmake_minimum_required(VERSION 3.16)
project(LGA_OpenInNukeX VERSION 1.77.0 LANGUAGES CXX)

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

# ── Nombres (convencion LGA, ver docs/Doc_Deploy_macOS.md del template) ──────
#   APP_NAME       el `.app` y el ejecutable adentro: empieza con `LGA`, con espacios
#   APP_MENU_NAME  CFBundleName: la barra de menu y el "About", SIN el prefijo `LGA`
# El nombre de ARCHIVO de los artefactos (ARTIFACT_NAME) vive en create_dmg.sh / deploy.sh.
if(APPLE)
    set(APP_NAME "LGA OpenInNukeX")
else()
    # En Windows el ejecutable NO lleva espacios: viaja por el registro, por lineas de
    # comando de asociacion de archivos y por el instalador.
    set(APP_NAME "LGA_OpenInNukeX")
endif()
set(APP_MENU_NAME "OpenInNukeX")

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
    src/lgaregistry.cpp
    src/lgaregistry.h
    src/appsettings.cpp
    src/appsettings.h
    src/i18n.cpp
    src/i18n.h
    src/nukebridge.cpp
    src/nukebridge.h
    src/dialogs.cpp
    src/dialogs.h
    src/dialogbutton.cpp
    src/dialogbutton.h
    src/dialogstyle.h
)

# El payload del Nuke Bridge: los `.py` que se instalan en `<.nuke>/LGA_OpenInNukeX/`.
# Viven en la raiz del repo porque son TAMBIEN lo que se publica como plugin de Nuke; el
# artefacto se los lleva adentro para poder instalarlos en una maquina sin el repo.
set(BRIDGE_PAYLOAD
    "${CMAKE_SOURCE_DIR}/../init.py"
    "${CMAKE_SOURCE_DIR}/../LGA_QtAdapter_OpenInNukeX.py"
)
foreach(_bridge_file ${BRIDGE_PAYLOAD})
    if(NOT EXISTS "${_bridge_file}")
        message(FATAL_ERROR
            "Falta un archivo del Nuke Bridge: ${_bridge_file}\n"
            "Sin el, la app compila pero el boton INSTALL no tiene que instalar.")
    endif()
endforeach()

add_executable(LGA_OpenInNukeX ${SOURCES})

set_target_properties(LGA_OpenInNukeX PROPERTIES OUTPUT_NAME "${APP_NAME}")

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

    # El payload del bridge va en `bridge/` al lado del .exe, que es donde lo busca
    # NukeBridge::payloadDirectory().
    add_custom_command(TARGET LGA_OpenInNukeX POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:LGA_OpenInNukeX>/bridge"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${BRIDGE_PAYLOAD} "$<TARGET_FILE_DIR:LGA_OpenInNukeX>/bridge"
        COMMENT "Copiando el payload del Nuke Bridge junto al ejecutable")
endif()

# ── macOS ─────────────────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(LGA_OpenInNukeX PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
        # CFBundleName va SIN el prefijo `LGA`: es la barra de menu, donde con la app abierta
        # el prefijo no aporta y ocupa. El nombre de ARCHIVO del bundle si lo lleva, porque
        # es lo unico que agrupa las apps LGA en Finder y en el Dock.
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_MENU_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        # GOTCHA: la propiedad se llama MACOSX_BUNDLE_GUI_IDENTIFIER, no ..._IDENTIFIER.
        # Con el nombre equivocado CMake no se queja y la sustitucion del plist queda vacia.
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.lga.openinnukex"
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

    # Payload del Nuke Bridge en Contents/Resources/bridge, que es donde lo busca
    # NukeBridge::payloadDirectory().
    target_sources(LGA_OpenInNukeX PRIVATE ${BRIDGE_PAYLOAD})
    set_source_files_properties(${BRIDGE_PAYLOAD} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources/bridge"
        # Sin esto CMake intenta COMPILAR los .py: los agrega a la lista de fuentes del
        # target y clang los rechaza por extension desconocida.
        HEADER_FILE_ONLY TRUE
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS LGA_OpenInNukeX
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
