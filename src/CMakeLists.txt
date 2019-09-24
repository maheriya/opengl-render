##############################################################################################
#
##############################################################################################
cmake_minimum_required(VERSION 3.12)
project(gl-render LANGUAGES CUDA CXX)

INCLUDE(FindPkgConfig)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)

# setup C++ flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -std=c++11 -Wno-write-strings")
set(CMAKE_CXX_FLAGS_DEBUG "-g -std=c++11 -Wno-write-strings")
include_directories(include)

# setup CUDA
find_package(CUDA 10 REQUIRED)
CUDA_SELECT_NVCC_ARCH_FLAGS(ARCH_FLAGS 6.1+PTX) # 6.1 for GTX 1080
LIST(APPEND CUDA_NVCC_FLAGS ${ARCH_FLAGS})

set(CUDA_NVCC_FLAGS_RELEASE ${CUDA_NVCC_FLAGS_RELEASE} -O3)
set(CUDA_NVCC_FLAGS_DEBUG   ${CUDA_NVCC_FLAGS_DEBUG}   -g -G --generate-line-info)
set(CMAKE_CUDA_STANDARD 11)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
include_directories(${CUDA_INCLUDE_DIRS})
message("== CUDA version: ${CUDA_VERSION}")
message("== system arch:  ${CMAKE_SYSTEM_PROCESSOR}")


# Find OpenCV
find_package( OpenCV 4 REQUIRED ) # not requiring EXACT sub-version
include_directories(${OpenCV_INCLUDE_DIRS})

# Find Freetype
find_package( Freetype 2 REQUIRED )
include_directories(${FREETYPE_INCLUDE_DIRS})

# Find Glib and Gstreamer
pkg_check_modules(GLIB_PKG glib-2.0 gobject-2.0 gio-2.0 REQUIRED)
include_directories(${GLIB_PKG_INCLUDE_DIRS})
find_package(Gstreamer REQUIRED ) 
include_directories(${GSTREAMER_INCLUDE_DIRS})

## Source files
set(SRCFILES
        cpp/main.cpp
        cpp/detection_window.cpp
    )


set(LIBS
    ${GSTREAMER_LIBRARIES}
    ${GLIB_PKG_LIBRARIES} 
    gstapp-1.0
    glfw;GL;GLEW
    ${OpenCV_LIBS}
    ${FREETYPE_LIBRARIES}
) 

## Executable to build
cuda_add_executable(${PROJECT_NAME} ${SRCFILES})
target_link_libraries(${PROJECT_NAME} ${LIBS})

##--cuda_add_executable(draw-cube cpp/draw_cube.cpp cpp/shader.cpp)
##--target_link_libraries(draw-cube ${LIBS})
##--
##--cuda_add_executable(draw-cube-chcolor cpp/draw_cube_change_color.cpp cpp/shader.cpp)
##--target_link_libraries(draw-cube-chcolor ${LIBS})
##--
##--cuda_add_executable(draw-texture cpp/draw_texture.cpp cpp/shader.cpp cpp/texture.cpp)
##--target_link_libraries(draw-texture ${LIBS})
##--
##--cuda_add_executable(movement cpp/movement.cpp cpp/shader.cpp cpp/texture.cpp cpp/controls.cpp)
##--target_link_libraries(movement ${LIBS})
##--
##--cuda_add_executable(draw-text2D cpp/draw_text2D.cpp cpp/shader.cpp cpp/texture.cpp cpp/controls.cpp cpp/objloader.cpp cpp/text2D.cpp)
##--target_link_libraries(draw-text2D ${LIBS})
##--
##--cuda_add_executable(image-viewer cpp/image_viewer.cpp)
##--target_link_libraries(image-viewer ${LIBS})
##--
##--cuda_add_executable(alpha-blending cpp/alpha_blending_with_text.cpp)
##--target_link_libraries(alpha-blending ${LIBS})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
