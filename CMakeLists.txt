cmake_minimum_required(VERSION 2.8.8)

include(ExternalProject)

project(cg2p2)

set(CMAKE_AUTOMOC on)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(OpenGL REQUIRED)
find_package(PNG REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui OpenGL)

ExternalProject_Add(
    dake
    GIT_REPOSITORY git://github.com/XanClic/dake.git
    GIT_TAG origin/master
    SOURCE_DIR dake
    CMAKE_COMMAND true
    BUILD_COMMAND make
    BINARY_DIR dake
    INSTALL_COMMAND true
)

ExternalProject_Get_Property(dake binary_dir)

include_directories(${binary_dir}/include ${PNG_INCLUDE})
link_directories(${binary_dir})

add_executable(cg2p2 main.cpp asf.cpp window.cpp render_output.cpp)
target_link_libraries(cg2p2 libdake.a ${OPENGL_LIBRARIES} ${PNG_LIBRARIES})
add_dependencies(cg2p2 dake)

set(CMAKE_CXX_FLAGS "-std=c++11 -O3 -g2 -Wall -Wextra -Wshadow")

qt5_use_modules(cg2p2 Core Gui OpenGL)
