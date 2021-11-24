# Minimum CMake required
cmake_minimum_required(VERSION 3.14)

project(MqttPublisherPlugin)

set(CMAKE_BUILD_TYPE Debug)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/../../src/")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FindPkgConfig)
find_package(aws-crt-cpp PATHS ~/sdk-cpp-workspace/build)
find_package(EventstreamRpc-cpp PATHS ~/sdk-cpp-workspace/build)
find_package(GreengrassIpc-cpp PATHS ~/sdk-cpp-workspace/build)

pkg_check_modules(GSTREAMER gstreamer-1.0 gstreamer-base-1.0)

#including GStreamer header files directory
include_directories(
        ${GLIB_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
        ${GSTREAMER_BASE_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GSTREAMER_LIBRARY_DIRS}
        ${GSTREAMER_BASE_LIBRARIES}
)

add_library(gstlookoutvisionmeta STATIC
        ../../src/gst/lookoutvisionmeta/gstlookoutvisionmeta.cc
)

add_library(gstmqttpublisher SHARED
        ./mqttpublisher/gstmqttpublisher.cc
)

add_library(GreengrassClient STATIC
        ./greengrass-client/GreengrassClient.cc
)
target_link_libraries(GreengrassClient
                        AWS::GreengrassIpc-cpp)

#linking Gstreamer library with target executable
target_link_libraries(gstmqttpublisher
                        gstlookoutvisionmeta
                        ${GSTREAMER_LIBRARIES}
                        GreengrassClient)
