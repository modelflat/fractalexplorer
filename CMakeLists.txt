cmake_minimum_required(VERSION 3.10)

project(FractalExplorer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)

add_executable(${PROJECT_NAME}

               app/App.cpp

               app/core/OpenCLBackend.hpp
               app/core/OpenCLBackend.cpp

               app/core/ComputableImage.hpp
               app/core/Evolution.hpp
               app/core/Utility.hpp

               app/core/clc/CLC_Sources.hpp
               app/core/clc/CLC_Sources.cpp
               app/core/clc/CLC_Random.hpp
               app/core/clc/CLC_Definitions.hpp
               app/core/clc/CLC_NewtonFractal.hpp

               app/core/glsl/GLSL_Render.hpp
               app/core/glsl/GLSL_RenderStatisticalClear.hpp
               app/core/glsl/GLSL_Sources.cpp
               app/core/glsl/GLSL_Sources.hpp

               app/ui/ComputableImageWidget.hpp)

include_directories(app/core)

include(cmake/QtDeploy.cmake)

include_directories(${ADDITIONAL_INCLUDES}) # TODO this is a workaround for using CLion IDE for highlighting

find_package(OpenCL)
include_directories(${OpenCL_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets ${OpenCL_LIBRARY})

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