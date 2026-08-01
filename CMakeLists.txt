cmake_minimum_required(VERSION 3.10)
project(BabyKeySmash C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(X11 REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(VORBISFILE REQUIRED vorbisfile)

set(RENDERER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opengl_depth_and_color_renderer/src/Library)

add_executable(babykeysmash
  src/main.cpp
  src/glx_window.c
  src/shadertoy.c
  src/sprites.cpp
  src/webcam.cpp
  src/audio_alsa.c
  src/soundbank.c
  ${RENDERER_DIR}/Rendering/ShaderPipeline/shader_loader.c
)

target_compile_definitions(babykeysmash PRIVATE
  USE_GLEW=1
  BKS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(babykeysmash PRIVATE ${OpenCV_INCLUDE_DIRS} ${X11_INCLUDE_DIR})

target_link_libraries(babykeysmash
  OpenGL::GL
  GLEW::GLEW
  ${X11_LIBRARIES}
  ${OpenCV_LIBS}
  ALSA::ALSA
  ${VORBISFILE_LIBRARIES}
  Threads::Threads
  m
)
