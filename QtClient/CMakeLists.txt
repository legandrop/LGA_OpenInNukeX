cmake_minimum_required(VERSION 3.16)
project(LGA_OpenInNukeX VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Asegurar que MOC se ejecute automáticamente
set(CMAKE_AUTOMOC ON)

# Crear el ejecutable
add_executable(LGA_OpenInNukeX
    src/main.cpp
    src/nukeopener.cpp
    src/nukeopener.h
    src/configwindow.cpp
    src/configwindow.h
    src/logger.cpp
    src/logger.h
)

# Enlazar con las librerías de Qt
target_link_libraries(LGA_OpenInNukeX 
    Qt6::Core
    Qt6::Widgets 
    Qt6::Network
)

# Configuración para Windows
if(WIN32)
    set_target_properties(LGA_OpenInNukeX PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Configurar el icono para Windows
    target_sources(LGA_OpenInNukeX PRIVATE resources/app.rc)
    enable_language(RC)
endif()

# Instalar el ejecutable
install(TARGETS LGA_OpenInNukeX
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) 