cmake_minimum_required(VERSION 3.10)

project(FractalExplorer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)

add_executable(${PROJECT_NAME}
               src/main.cpp
               )

include(cmake/QtDeploy.cmake)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets)

if (CMAKE_BUILD_TYPE MATCHES Release)
    create_target_installer(
        ${PROJECT_NAME}
        ${PROJECT_SOURCE_DIR}/installer/installer.xml
        ${PROJECT_SOURCE_DIR}/installer/package.xml
        com.github.modelflat.fractalexplorer
        "--no-angle --no-webkit2 --no-translations --no-system-d3d-compiler --no-plugins"
    )
else()
    deploy_target(${PROJECT_NAME} "")
endif()