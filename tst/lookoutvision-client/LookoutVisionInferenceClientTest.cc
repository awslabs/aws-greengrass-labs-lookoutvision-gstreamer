// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gst/gst.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "Inference_mock.grpc.pb.h"
#include "lookoutvision-client/LookoutVisionInferenceClient.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionresult.h"
#include "utils/test-server/TestServer.h"

using ::testing::HasSubstr;

using namespace AWS::LookoutVision;
using namespace testing;

class LookoutVisionInferenceClientTest : public testing::Test {
protected:
    GstElement *pipeline = nullptr;
    GstBus *bus = nullptr;
    TestServer* grpc_server = nullptr;

    void TearDown() override {
        if (bus) {
            gst_object_unref(bus);
        }
        if (pipeline) {
            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(pipeline);
        }
        if (grpc_server) {
            grpc_server->StopServer();
        }
    }
};

TEST_F(LookoutVisionInferenceClientTest, grpc_calls_throw_exception_test) {
    MockEdgeAgentStub mock_stub;
    ON_CALL(mock_stub, StartModel(_,_,_)).WillByDefault(Throw(std::runtime_error("StartModel failed")));
    ON_CALL(mock_stub, DescribeModel(_,_,_)).WillByDefault(Throw(std::runtime_error("DescribeModel failed")));
    ON_CALL(mock_stub, DetectAnomalies(_,_,_)).WillByDefault(Throw(std::runtime_error("DetectAnomalies failed")));

    LookoutVisionInferenceClient* inference_client = new LookoutVisionInferenceClient(&mock_stub);
    LookoutVisionInferenceClient::OperationStatus status = inference_client->StartModel("SampleModel", 5);
    ASSERT_EQ(status, LookoutVisionInferenceClient::OperationStatus::FAILED);

    guint8* buffer = new guint8[120]{};
    GstLookoutVisionResult* result = inference_client->DetectAnomalies("SampleModel", buffer, 120, 5, 8);
    ASSERT_EQ(result->error_message, "DetectAnomalies failed");
}

#ifdef SHARED_MEMORY
TEST_F(LookoutVisionInferenceClientTest, inference_on_large_size_frame_with_shared_memory_test) {
    testing::internal::CaptureStdout();

    grpc_server = new TestServer();
    grpc_server->RunServerInBackground("0.0.0.0:50051", "RUNNING");

    GstElement *source, *sink, *capsfilter, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    capsfilter = gst_element_factory_make("capsfilter", "caps");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(capsfilter, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, capsfilter, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);
    g_object_set(capsfilter, "caps", gst_caps_new_simple("video/x-raw",
                "width", G_TYPE_INT, 2000,
                "height", G_TYPE_INT, 2000,
                "format", G_TYPE_STRING, "RGB",
                NULL), NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-component", "SampleModel",
                "model-status-timeout", 180, NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    ASSERT_NE(ret, GST_STATE_CHANGE_FAILURE);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != NULL) {
        switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_ERROR:
                FAIL();
            case GST_MESSAGE_EOS:
                break;
        }
        gst_message_unref(msg);
    }

    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("Is Anomalous?"));
    ASSERT_THAT(output, HasSubstr("Confidence:"));
}

TEST_F(LookoutVisionInferenceClientTest, shm_open_error_test) {
    //make shm read-only so that it can't be opened by inference client
    int shm_fd = shm_open("/gstreamer-lookoutvision-bitmap", O_CREAT | O_RDONLY, 0444);

    std::string error;
    try {
        gst_element_factory_make("lookoutvision", "infer");
    } catch (std::runtime_error& e) {
        error = e.what();
    }

    close(shm_fd);
    shm_unlink("/gstreamer-lookoutvision-bitmap");

    ASSERT_EQ(error, "shm_open failed");
}
#endif

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    testing::InitGoogleTest();
    RUN_ALL_TESTS();

    return 0;
}
