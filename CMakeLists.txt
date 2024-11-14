cmake_minimum_required(VERSION 3.14.7)

project(ad2)

option(AD_TEST "Whether or not to build the test program." OFF)

find_package(OpenCV CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)

add_library(ad-pipeline
  pipeline.h
  pipeline.cpp
  report.h
  report.cpp)

target_link_libraries(ad-pipeline PUBLIC ${OpenCV_LIBRARIES})

add_executable(anomaly-detect
  main.cpp)

target_link_libraries(anomaly-detect
  PUBLIC
    ad-pipeline
    cxxopts::cxxopts
    spdlog::spdlog
    libuv::uv_a)

if(AD_TEST)

  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()

  include(FetchContent)

  set(UIKIT_MAIN ON CACHE BOOL "" FORCE)
  FetchContent_Declare(uikit URL "https://github.com/tay10r/uikit/archive/refs/heads/main.zip")
  FetchContent_MakeAvailable(uikit)

  add_executable(ad-test WIN32
    test.cpp)

  target_link_libraries(ad-test
    PUBLIC
      uikit::uikit
      uikit::main
      ad-pipeline)

endif()
