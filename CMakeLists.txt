# Minimum CMake required
cmake_minimum_required(VERSION 3.15)

project(LookoutVisionPlugin)

set(CMAKE_BUILD_TYPE Debug)

include_directories("${PROJECT_SOURCE_DIR}/src")

# Add shared memory option
option(USE_SHARED_MEMORY "Use shared memory?" OFF)
if (USE_SHARED_MEMORY)
  add_definitions(-DSHARED_MEMORY)
endif()

# Protobuf-compiler
set(protoc $<TARGET_FILE:protoc>)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# gRPC
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc.git
  GIT_TAG "v1.34.0")
FetchContent_MakeAvailable(gRPC)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

# Proto file
get_filename_component(hw_proto "src/lookoutvision-client/Inference.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/Inference.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/Inference.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/Inference.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/Inference.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${protoc}
      ARGS --grpc_out=generate_mock_code=true:"${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(LookoutVisionInferenceClient STATIC "src/lookoutvision-client/LookoutVisionInferenceClient.cc"
  ${hw_proto_srcs}
  ${hw_grpc_srcs})
  set_property(TARGET LookoutVisionInferenceClient PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(LookoutVisionInferenceClient
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  libprotobuf)

include(FindPkgConfig)

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
        src/gst/lookoutvisionmeta/gstlookoutvisionmeta.cc
)

add_library(gstlookoutvision SHARED
        src/gst/lookoutvision/gstlookoutvision.cc
)

#linking Gstreamer library with target executable
target_link_libraries(gstlookoutvision
                        gstlookoutvisionmeta
                        ${GSTREAMER_LIBRARIES}
                        LookoutVisionInferenceClient)

option(BUILD_TEST "Build the tests" OFF)
if(BUILD_TEST)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -fprofile-arcs -ftest-coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

  FetchContent_Declare(
          googletest
          GIT_REPOSITORY https://github.com/google/googletest.git
          GIT_TAG "main")
  FetchContent_MakeAvailable(googletest)
  set(BUILD_GMOCK ON)

  file(DOWNLOAD https://raw.githubusercontent.com/meekrosoft/fff/master/fff.h ./fff.h)

  add_subdirectory(tst)
endif()
