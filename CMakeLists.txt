cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(ai-mouse-follower VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" ON)
option(USE_CUDA "Enable CUDA support for ONNX Runtime" ON)

include(compilerconfig)
include(defaults)
include(helpers)
include(ExternalProject)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

set(ONNXRUNTIME_VERSION "1.24.1")
set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}")

if(WIN32)
  if(USE_CUDA)
    set(ONNXRUNTIME_FILENAME "onnxruntime-win-x64-gpu-${ONNXRUNTIME_VERSION}.zip")
  else()
    set(ONNXRUNTIME_FILENAME "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip")
  endif()
elseif(APPLE)
  set(ONNXRUNTIME_FILENAME "onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}.zip")
else()
  set(ONNXRUNTIME_FILENAME "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz")
endif()

set(ONNXRUNTIME_DOWNLOAD_URL "${ONNXRUNTIME_URL}/${ONNXRUNTIME_FILENAME}")

ExternalProject_Add(
  onnxruntime
  URL "${ONNXRUNTIME_DOWNLOAD_URL}"
  DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloads"
  PREFIX "${CMAKE_BINARY_DIR}/onnxruntime"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(onnxruntime SOURCE_DIR)
set(ONNXRUNTIME_INCLUDE_DIR "${SOURCE_DIR}/include")
set(ONNXRUNTIME_LIB_DIR "${SOURCE_DIR}/lib")

if(WIN32)
  set(ONNXRUNTIME_LIBS "${ONNXRUNTIME_LIB_DIR}/onnxruntime.lib")
elseif(APPLE)
  set(ONNXRUNTIME_LIBS "${ONNXRUNTIME_LIB_DIR}/libonnxruntime.dylib")
else()
  set(ONNXRUNTIME_LIBS "${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so")
endif()

add_dependencies(${CMAKE_PROJECT_NAME} onnxruntime)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${ONNXRUNTIME_LIBS}")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.c src/yolo-filter.c)

if(WIN32)
  install(FILES "${ONNXRUNTIME_LIB_DIR}/onnxruntime.dll" DESTINATION "obs-plugins/${_name}")
  if(USE_CUDA)
    install(FILES "${ONNXRUNTIME_LIB_DIR}/onnxruntime_providers_cuda.dll" DESTINATION "obs-plugins/${_name}")
    install(FILES "${ONNXRUNTIME_LIB_DIR}/onnxruntime_providers_shared.dll" DESTINATION "obs-plugins/${_name}")
  endif()
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
