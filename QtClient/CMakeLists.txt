cmake_minimum_required(VERSION 3.16)
project(LGA_OpenInNukeX_Qt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# Asegurar que MOC se ejecute automáticamente
set(CMAKE_AUTOMOC ON)

# Crear el ejecutable
add_executable(LGA_OpenInNukeX_Qt
    main.cpp
    nukeopener.cpp
    nukeopener.h
    configwindow.cpp
    configwindow.h
    logger.cpp
    logger.h
)

# Enlazar con las librerías de Qt
target_link_libraries(LGA_OpenInNukeX_Qt 
    Qt6::Core
    Qt6::Widgets 
    Qt6::Network
)

# Configuración para Windows
if(WIN32)
    set_target_properties(LGA_OpenInNukeX_Qt PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Instalar el ejecutable
install(TARGETS LGA_OpenInNukeX_Qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) 