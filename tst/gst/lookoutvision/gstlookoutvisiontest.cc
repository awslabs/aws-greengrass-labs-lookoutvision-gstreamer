#include <gst/gst.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gst/check/gstharness.h>
#include "utils/test-server/TestServer.h"

using ::testing::HasSubstr;

class gstlookoutvisiontest : public testing::Test {
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

TEST_F(gstlookoutvisiontest, plugin_init_test) {
    GstElement* lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    ASSERT_NE(lookoutvision, nullptr);
    gst_object_unref(lookoutvision);
}

TEST_F(gstlookoutvisiontest, element_linking_test) {
    GstElement *source, *sink, *lookoutvision;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));
}

TEST_F(gstlookoutvisiontest, pipeline_run_without_server_test) {
    testing::internal::CaptureStdout();

    GstElement *source, *sink, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

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
    ASSERT_THAT(output, HasSubstr("Inference call failed"));
}

TEST_F(gstlookoutvisiontest, pipeline_run_with_server_test) {
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
                "width", G_TYPE_INT, 1280,
                "height", G_TYPE_INT, 720,
                "format", G_TYPE_STRING, "RGB",
                NULL), NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-component", "SampleModel",
                "model-status-timeout", 180, NULL);
    /* Verify LookoutVision properties */
    gchar *server_socket, *model_component;
    guint model_status_timeout;
    g_object_get(lookoutvision, "server-socket", &server_socket, "model-component", &model_component,
                "model-status-timeout", &model_status_timeout, NULL);
    ASSERT_TRUE(g_str_equal(server_socket, "0.0.0.0:50051") && g_str_equal(model_component, "SampleModel")
                            && model_status_timeout == 180);

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

TEST_F(gstlookoutvisiontest, pipeline_invalid_input_type_test) {
    GstElement *source, *sink, *capsfilter, *lookoutvision;

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

    g_object_set(capsfilter, "caps", gst_caps_new_simple("video/x-raw",
                "width", G_TYPE_INT, 1280,
                "height", G_TYPE_INT, 720,
                "format", G_TYPE_STRING, "BGR",
                NULL), NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, lookoutvision, sink, NULL);
    ASSERT_FALSE(gst_element_link_many(source, capsfilter, lookoutvision, sink, NULL));
}

TEST_F(gstlookoutvisiontest, pipeline_run_without_server_start_model_error_test) {
    testing::internal::CaptureStdout();

    GstElement *source, *sink, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-status-timeout", 3,
                "model-component", "SampleModel", NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    ASSERT_NE(ret, GST_STATE_CHANGE_FAILURE);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    bool errored = false;
    if (msg != NULL) {
        switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_ERROR:
                errored = true;
                break;
            case GST_MESSAGE_EOS:
                break;
        }
        gst_message_unref(msg);
    }

    ASSERT_TRUE(errored);
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("StartModel returned error 14: failed to connect to all addresses"));
    ASSERT_THAT(output, HasSubstr("DescribeModel failed with error 14: failed to connect to all addresses"));
    ASSERT_THAT(output, HasSubstr("Model didn't reach RUNNING state"));
}

TEST_F(gstlookoutvisiontest, pipeline_run_with_server_start_model_failed_test) {
    testing::internal::CaptureStdout();

    grpc_server = new TestServer();
    grpc_server->RunServerInBackground("0.0.0.0:50051", "FAILED");

    GstElement *source, *sink, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-status-timeout", 10,
                "model-component", "SampleModel", NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    ASSERT_NE(ret, GST_STATE_CHANGE_FAILURE);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    bool errored = false;
        if (msg != NULL) {
            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    errored = true;
                    break;
                case GST_MESSAGE_EOS:
                    break;
            }
        gst_message_unref(msg);
    }

    ASSERT_TRUE(errored);
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("Model didn't reach RUNNING state"));
}

TEST_F(gstlookoutvisiontest, pipeline_run_with_server_start_model_timeout_test) {
    testing::internal::CaptureStdout();

    grpc_server = new TestServer();
    grpc_server->RunServerInBackground("0.0.0.0:50051", "RUNNING");

    GstElement *source, *sink, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-status-timeout", 0,
                 "model-component", "SampleModel", NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    ASSERT_NE(ret, GST_STATE_CHANGE_FAILURE);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    bool errored = false;
        if (msg != NULL) {
            switch (GST_MESSAGE_TYPE (msg)) {
                case GST_MESSAGE_ERROR:
                    errored = true;
                    break;
                case GST_MESSAGE_EOS:
                    break;
            }
        gst_message_unref(msg);
    }

    ASSERT_TRUE(errored);
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("Model didn't reach RUNNING state"));
}

TEST_F(gstlookoutvisiontest, pipeline_update_server_start_model_test) {
    testing::internal::CaptureStdout();

    grpc_server = new TestServer();
    grpc_server->RunServerInBackground("0.0.0.0:50051", "RUNNING");

    GstElement *source, *sink, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-component", "SampleModel", NULL);
    /* Restart server with new address and modify the property */
    grpc_server->StopServer();
    grpc_server->RunServerInBackground("0.0.0.0:50052", "RUNNING");
    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50052", NULL);

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

TEST_F(gstlookoutvisiontest, pipeline_update_server_start_model_error_test) {
    testing::internal::CaptureStdout();

    grpc_server = new TestServer();
    grpc_server->RunServerInBackground("0.0.0.0:50051", "RUNNING");

    GstElement *source, *sink, *lookoutvision;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-component", "SampleModel",
                "model-status-timeout", 5, NULL);
    /* Modify the server-socket property */
    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50052", NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    ASSERT_NE(ret, GST_STATE_CHANGE_FAILURE);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    bool errored = false;
    if (msg != NULL) {
        switch (GST_MESSAGE_TYPE (msg)) {
            case GST_MESSAGE_ERROR:
                errored = true;
                break;
            case GST_MESSAGE_EOS:
                break;
        }
        gst_message_unref(msg);
    }

    ASSERT_TRUE(errored);
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("StartModel returned error 14: failed to connect to all addresses"));
    ASSERT_THAT(output, HasSubstr("DescribeModel failed with error 14: failed to connect to all addresses"));
    ASSERT_THAT(output, HasSubstr("Model didn't reach RUNNING state"));
}

TEST_F(gstlookoutvisiontest, no_width_in_caps_test) {
    testing::internal::CaptureStdout();

    GstHarness *harness = gst_harness_new("lookoutvision");

    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "I420",
                                        "framerate", GST_TYPE_FRACTION, 25, 1,
                                        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                        "height", G_TYPE_INT, 240,
                                        NULL);
    gst_harness_set_src_caps(harness, caps);

    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("no dimensions"));

    gst_harness_teardown(harness);
}

TEST_F(gstlookoutvisiontest, no_height_in_caps_test) {
    testing::internal::CaptureStdout();

    GstHarness *harness = gst_harness_new("lookoutvision");

    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "I420",
                                        "framerate", GST_TYPE_FRACTION, 25, 1,
                                        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                        "width", G_TYPE_INT, 320,
                                        NULL);
    gst_harness_set_src_caps(harness, caps);

    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("no dimensions"));

    gst_harness_teardown(harness);
}

TEST_F(gstlookoutvisiontest, no_dimensions_in_caps_test) {
    testing::internal::CaptureStdout();

    GstHarness *harness = gst_harness_new("lookoutvision");

    GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                        "format", G_TYPE_STRING, "I420",
                                        "framerate", GST_TYPE_FRACTION, 25, 1,
                                        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                                        NULL);
    gst_harness_set_src_caps(harness, caps);

    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_THAT(output, HasSubstr("no dimensions"));

    gst_harness_teardown(harness);
}

TEST_F(gstlookoutvisiontest, pipeline_read_inference_meta_test) {
    testing::internal::CaptureStdout();

    grpc_server = new TestServer();
    grpc_server->RunServerInBackground("0.0.0.0:50051", "RUNNING");

    GstElement *source, *sink, *lookoutvision, *consumer;
    GstMessage *msg;
    GstStateChangeReturn ret;

    source = gst_element_factory_make("videotestsrc", "source");
    lookoutvision = gst_element_factory_make("lookoutvision", "infer");
    consumer = gst_element_factory_make("inferenceconsumer", "consumer");
    sink = gst_element_factory_make("autovideosink", "sink");
    pipeline = gst_pipeline_new("pipeline");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(lookoutvision, nullptr);
    ASSERT_NE(consumer, nullptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_NE(pipeline, nullptr);

    gst_bin_add_many(GST_BIN(pipeline), source, lookoutvision, consumer, sink, NULL);
    ASSERT_TRUE(gst_element_link_many(source, lookoutvision, consumer, sink, NULL));

    g_object_set(source, "pattern", 0, "num-buffers", 1, NULL);

    g_object_set(lookoutvision, "server-socket", "0.0.0.0:50051", "model-component", "SampleModel", NULL);

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
    ASSERT_THAT(output, HasSubstr("Detect Anomaly Result - Is Anomalous?"));
    ASSERT_THAT(output, HasSubstr("Confidence:"));
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    testing::InitGoogleTest();
    RUN_ALL_TESTS();

    return 0;
}
