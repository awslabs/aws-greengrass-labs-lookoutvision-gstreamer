include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

pkg_check_modules(GST_CHECK REQUIRED gstreamer-check-1.0)

add_library(gstinferenceconsumer SHARED
        utils/inferenceconsumer/gstinferenceconsumer.cc
        )
target_link_libraries(gstinferenceconsumer
        gstlookoutvisionmeta
        ${GSTREAMER_LIBRARIES})

include_directories("${PROJECT_BINARY_DIR}")

add_custom_command(
        OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
        COMMAND ${protoc}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
        DEPENDS "${hw_proto}")

add_library(TestServer STATIC "utils/test-server/TestServer.cc"
        ${hw_proto_srcs}
        ${hw_grpc_srcs})
set_property(TARGET TestServer PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(TestServer
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        libprotobuf)

add_executable(gstlookoutvisionmetatest gst/lookoutvisionmeta/gstlookoutvisionmetatest.cc)
add_executable(gstlookoutvisiontest gst/lookoutvision/gstlookoutvisiontest.cc)
add_executable(LookoutVisionInferenceClientTest lookoutvision-client/LookoutVisionInferenceClientTest.cc)

target_link_libraries( gstlookoutvisionmetatest
        gstlookoutvisionmeta
        ${GSTREAMER_LIBRARIES}
        gtest)

target_link_libraries( gstlookoutvisiontest
        ${GSTREAMER_LIBRARIES}
        ${GST_CHECK_LIBRARIES}
        TestServer
        gtest
        gmock)

target_link_libraries( LookoutVisionInferenceClientTest
        ${GSTREAMER_LIBRARIES}
        LookoutVisionInferenceClient
        TestServer
        gtest
        gmock)

enable_testing()

add_test(NAME gstlookoutvisionmetatest COMMAND gstlookoutvisionmetatest)
add_test(NAME gstlookoutvisiontest COMMAND gstlookoutvisiontest --gst-plugin-path=../)
add_test(NAME LookoutVisionInferenceClientTest COMMAND LookoutVisionInferenceClientTest --gst-plugin-path=../)
