cmake_minimum_required(VERSION 3.21)
project(DepthEye VERSION 0.1.0 LANGUAGES C CXX)

# additional cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")

# system/version parsing (preserve existing behavior)
string(REPLACE "\n" "" SYSTEM_VERSION ${CMAKE_SYSTEM_VERSION})
string (REGEX REPLACE "^([0-9]+).*" "\\1" SYSTEM_MAJOR_VERSION "${SYSTEM_VERSION}")
string (REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" SYSTEM_MINOR_VERSION "${SYSTEM_VERSION}")
string (REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" SYSTEM_PATCH_VERSION ${SYSTEM_VERSION})

# dependencies
find_package(PkgConfig QUIET)

message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME} VERSION=${SYSTEM_VERSION}(${SYSTEM_MAJOR_VERSION}-${SYSTEM_MINOR_VERSION}-${SYSTEM_PATCH_VERSION})")
message(STATUS "ARCH = ${CMAKE_SYSTEM_PROCESSOR}")

# default containers for includes/libs
set(COMMON_INCLUDE "")
set(COMMON_LIBS "")
set(PointCloud "")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(COMMON_LIBS /usr/local/lib ./lib)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    message(STATUS "Build for aarch64")
    add_compile_options(-pthread -std=c++11 -fPIC -ffast-math -fpermissive)
    list(APPEND COMMON_INCLUDE /usr/include/libusb-1.0 ./include ./libs/ubuntu_arm/include/pointcloud-1.3.0/)
    set(PointCloud ./libs/ubuntu_arm/lib)
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
    message(STATUS "Build for armv7l")
    add_compile_options(-pthread -std=c++11 -fPIC -ffast-math)
    list(APPEND COMMON_INCLUDE /usr/include/libusb-1.0 ./include ./libs/rasp4/include/pointcloud-1.0.0/)
    set(PointCloud ./libs/rasp4/lib)
    message(STATUS "use PointCloud SDK ./libs/rasp4/lib")
  else()
    message(STATUS "Build for x86_64")
    add_compile_options(-msse2 -pthread -std=c++11 -fPIC -ffast-math)
    list(APPEND COMMON_INCLUDE /usr/include/libusb-1.0 ./include ./libs/ubuntu/include/pointcloud-3.4.0/)
    set(PointCloud ./libs/ubuntu/lib)
  endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  add_compile_options(-msse2 -std=c++11 -fPIC -ffast-math)
  set(COMMON_LIBS /usr/local/lib ./lib)
  list(APPEND COMMON_INCLUDE /usr/include/libusb-1.0 ./include ./libs/macos/include/pointcloud-1.0.0/)
  set(PointCloud ./libs/macos/lib)
  message(STATUS "use PointCloud SDK ./libs/macos/lib")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(WINDOWS 1)
  set(PointCloud ./libs/windows/lib)

  # MSVC compile/link flags and preprocessor defines
  if(MSVC)
    add_compile_options(/fp:fast /W0 /bigobj /arch:AVX2)
    add_compile_definitions(WINDOWS EnableCyAPI _CRT_SECURE_NO_WARNINGS DLIB_HAVE_AVX2)
    # legacy warning suppression passed to compiler; avoid global ADD_DEFINITIONS
  endif()

  list(APPEND COMMON_INCLUDE
    ./libs/windows/include/pointcloud-1.0.0/
    "${CMAKE_SOURCE_DIR}/3party/windows/libusb1.0.0/include"
    Util/
  )

  if(NOT MSVC_VERSION GREATER 1800)
    set(COMMON_LIBS setupapi strmiids strmbase Cfgmgr32 Version)
  else()
    set(COMMON_LIBS setupapi strmiids strmbase Cfgmgr32 Version legacy_stdio_definitions)
  endif()

else()
  message(FATAL_ERROR "This project does not support platform: ${CMAKE_SYSTEM_NAME}")
endif()

# Output directories
set(BINDIR bin/)
set(LIBDIR lib/)
set(ARCDIR lib/)
set(SHAREDIR share/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${ARCDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${BINDIR})

# Global include/link directories (kept for backward compatibility)
include_directories(${COMMON_INCLUDE})
link_directories("/usr/local/lib" ${COMMON_LIBS} ${PointCloud})

# Robust GLFW detection:
# Prefer config-mode imported targets (e.g. vcpkg config-mode: glfw3::glfw),
# fall back to legacy FindGLFW3 module in cmake_modules which sets GLFW3_LIBRARIES.
find_package(glfw3 CONFIG QUIET)
if(TARGET glfw OR TARGET "glfw3::glfw" OR TARGET glfw3)
  if(TARGET glfw)
    set(GLFW3_IMPORTED_TARGET glfw)
  elseif(TARGET "glfw3::glfw")
    set(GLFW3_IMPORTED_TARGET glfw3::glfw)
  else()
    set(GLFW3_IMPORTED_TARGET glfw3)
  endif()
  set(GLFW3_LIBRARIES ${GLFW3_IMPORTED_TARGET})
  set(GLFW3_LIBRARY ${GLFW3_IMPORTED_TARGET})
  message(STATUS "Using imported GLFW target: ${GLFW3_IMPORTED_TARGET}")
else()
  # Try legacy Find module (searches cmake_modules/FindGLFW3.cmake)
  find_package(GLFW3 QUIET)
  if(GLFW3_LIBRARIES)
    message(STATUS "Found GLFW (legacy Find): ${GLFW3_LIBRARIES}")
  else()
    message(STATUS "GLFW not found via config-mode or Find module; examples that require GLFW may fail unless you install glfw3 (vcpkg or system) or set GLFW3_LIBRARIES.")
  endif()
endif()

#### Packaging related information -- common to both platforms
set(CPACK_PACKAGE_NAME "Depth Eye SDK")
set(CPACK_PACKAGE_VERSION "0.1")
set(CPACK_PACKAGE_CONTACT "dev@pointcloud.ai")
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_PACKAGE_VENDOR "pointcloud.ai")
set(CPACK_SOURCE_GENERATOR "Developer")

# Subprojects / examples
add_subdirectory(examples/DepthMeasure)
#add_subdirectory(examples/Tof_GLFW)