// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/**
 * SECTION:element-lookoutvision
 *
 * Performs inference on frames using the Lookout for Vision Edge Agent
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 *   gst-launch-1.0
 *     videotestsrc num-buffers=1 pattern="ball"
 *   ! 'video/x-raw, format=RGB, width=1280, height=720'
 *   ! videoconvert
 *   ! lookoutvision server-socket="unix:///tmp/aws.iot.lookoutvision.EdgeAgent.sock" model-component="SampleModel"
 *   ! videoconvert
 *   ! jpegenc
 *   ! filesink location=./anomaly.jpg
 * ]|
 * </refsect2>
 */

#include <gst/gst.h>
#include "gstlookoutvision.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionmeta.h"
#include "lookoutvision-client/LookoutVisionInferenceClient.h"

GST_DEBUG_CATEGORY_STATIC(gst_lookout_vision_debug);
#define GST_CAT_DEFAULT gst_lookout_vision_debug

enum {
    PROP_0,
    PROP_SERVER_SOCKET,
    PROP_MODEL_COMPONENT,
    PROP_MODEL_STATUS_TIMEOUT
};

/* Inputs and outputs */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("video/x-raw, format={RGB}")
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("video/x-raw, format={RGB}")
);

#define gst_lookout_vision_parent_class parent_class
G_DEFINE_TYPE(GstLookoutVision, gst_lookout_vision, GST_TYPE_ELEMENT);

static void gst_lookout_vision_set_property(GObject * object, guint prop_id,
                                            const GValue * value, GParamSpec * pspec);
static void gst_lookout_vision_get_property(GObject * object, guint prop_id,
                                            GValue * value, GParamSpec * pspec);
static void gst_lookout_vision_finalize(GObject *object);

static gboolean gst_lookout_vision_sink_event(GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_lookout_vision_chain(GstPad * pad, GstObject * parent, GstBuffer * buf);

/* initialize the lookoutvision class */
static void gst_lookout_vision_class_init(GstLookoutVisionClass * klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_lookout_vision_set_property;
    gobject_class->get_property = gst_lookout_vision_get_property;
    gobject_class->finalize = gst_lookout_vision_finalize;

    g_object_class_install_property(gobject_class, PROP_SERVER_SOCKET,
                                    g_param_spec_string("server-socket", "Server Socket", "Socket for gRPC server ?",
                                                        "unix:///tmp/aws.iot.lookoutvision.EdgeAgent.sock",
                                                        G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_MODEL_COMPONENT,
                                    g_param_spec_string("model-component", "Model Component", "Model Component", "foo",
                                                        G_PARAM_READWRITE));
    g_object_class_install_property(gobject_class, PROP_MODEL_STATUS_TIMEOUT,
                                    g_param_spec_uint("model-status-timeout", "Model Status Timeout",
                                                      "Timeout in seconds to wait for Model Status", 0, 600, 180,
                                                      G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class,
                                         "LookoutVision",
                                         "GstLookoutVision",
                                         "Lookout for Vision inference GStreamer plugin",
                                         "Amazon");

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}

static GstPadProbeReturn pad_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    if (GST_EVENT_CAPS == GST_EVENT_TYPE(event)) {
        GstCaps * caps = gst_caps_new_any();
        int width, height;
        gst_event_parse_caps(event, &caps);
        GstStructure *s = gst_caps_get_structure(caps, 0);
        if (!gst_structure_get_int(s, "width", &width)
            || !gst_structure_get_int(s, "height", &height)) {
            g_print("no dimensions\n");
            return GST_PAD_PROBE_REMOVE;
        }
        GstLookoutVision* filter = (GstLookoutVision*) user_data;
        filter->width = width;
        filter->height = height;
    }
    return GST_PAD_PROBE_OK;
}

/* 
 * initialize the new element
 * instantiate pads and add them to element
 * set pad callback functions
 * initialize instance structure
 */
static void gst_lookout_vision_init(GstLookoutVision * filter) {
    filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_lookout_vision_sink_event));
    gst_pad_set_chain_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_lookout_vision_chain));
    GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
    gst_element_add_pad(GST_ELEMENT (filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS(filter->srcpad);
    gst_element_add_pad(GST_ELEMENT (filter), filter->srcpad);

    gst_pad_add_probe(filter->sinkpad, GST_PAD_PROBE_TYPE_EVENT_BOTH, pad_probe, filter, nullptr);

    // Set default properties
    filter->model_component = NULL;
    filter->model_status_timeout = 180;
    filter->server_socket = g_strdup("unix:///tmp/aws.iot.lookoutvision.EdgeAgent.sock");
    filter->inference_client = new LookoutVisionInferenceClient(filter->server_socket);
}

static void gst_lookout_vision_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
    GstLookoutVision *filter = GST_LOOKOUTVISION(object);

    switch (prop_id) {
        case PROP_SERVER_SOCKET:
            g_free(filter->server_socket);
            filter->server_socket = g_strdup(g_value_get_string(value));
            filter->inference_client->setServerSocket(filter->server_socket);
            if (filter->model_component) {
                if (filter->inference_client->StartModel(filter->model_component, filter->model_status_timeout)
                        != LookoutVisionInferenceClient::OperationStatus::SUCCESSFUL) {
                    GST_ELEMENT_ERROR(filter, LIBRARY, FAILED, (NULL), ("Failed to start model"));
                }
            }
            break;
        case PROP_MODEL_COMPONENT:
            if (filter->inference_client->StartModel(g_value_get_string(value), filter->model_status_timeout)
                    != LookoutVisionInferenceClient::OperationStatus::SUCCESSFUL) {
                GST_ELEMENT_ERROR(filter, LIBRARY, FAILED, (NULL), ("Failed to start model"));
            } else {
                g_free(filter->model_component);
                filter->model_component = g_strdup(g_value_get_string(value));
            }
            break;
        case PROP_MODEL_STATUS_TIMEOUT:
            filter->model_status_timeout = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_lookout_vision_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
    GstLookoutVision *filter = GST_LOOKOUTVISION(object);

    switch (prop_id) {
        case PROP_SERVER_SOCKET:
            g_value_set_string(value, filter->server_socket);
            break;
        case PROP_MODEL_COMPONENT:
            g_value_set_string(value, filter->model_component);
            break;
        case PROP_MODEL_STATUS_TIMEOUT:
            g_value_set_uint(value, filter->model_status_timeout);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_lookout_vision_finalize(GObject *object) {
    GstLookoutVision *filter = GST_LOOKOUTVISION(object);
    if (filter) {
        GST_DEBUG_OBJECT(filter, "finalize");
        delete filter->inference_client;
        filter->inference_client = NULL;
        g_free(filter->server_socket);
        filter->server_socket = NULL;
        g_free(filter->model_component);
        filter->model_component = NULL;
    }
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* this function handles sink events */
static gboolean gst_lookout_vision_sink_event(GstPad * pad, GstObject * parent, GstEvent * event) {
    GST_LOG_OBJECT(GST_LOOKOUTVISION(parent), "Received %s event: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(event), event);
    return gst_pad_event_default(pad, parent, event);
}

/* chain function */
static GstFlowReturn gst_lookout_vision_chain(GstPad * pad, GstObject * parent, GstBuffer * buf) {
    GstLookoutVision *filter;
    filter = GST_LOOKOUTVISION(parent);

    // Extract image from gstbuffer
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    // Send image to inference server and get response
    GstLookoutVisionResult* inference_result;
    if (filter->model_component) {
        inference_result = filter->inference_client->DetectAnomalies(filter->model_component, map.data, map.size,
                                                                     filter->width, filter->height);
    } else {
        inference_result = new GstLookoutVisionResult{0, 0, GstLookoutVisionResultStatus::FAILED,
                                                      "No value set for model-component"};
    }

    gst_buffer_add_lookout_vision_meta(buf, inference_result);
    if (inference_result->result_status == GstLookoutVisionResultStatus::SUCCESSFUL) {
        std::cout << "Is Anomalous? " << inference_result->is_anomalous
                << ", Confidence: " << inference_result->confidence << std::endl;
    } else  {
        std::cout << "Inference call failed" << std::endl;
    }

    // Free memory
    gst_buffer_unmap(buf, &map);
    delete inference_result;

    return gst_pad_push(filter->srcpad, buf);
}

/* 
 * entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean lookoutvision_init(GstPlugin * lookoutvision) {
    GST_DEBUG_CATEGORY_INIT(gst_lookout_vision_debug, "lookoutvision", 0, "Lookout for Vision inference plugin");

    return gst_element_register (lookoutvision, "lookoutvision", GST_RANK_NONE, GST_TYPE_LOOKOUTVISION);
}

#ifndef PACKAGE
#define PACKAGE "lookoutvision"
#endif

GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        lookoutvision,
        "Lookout for Vision inference plugin",
        lookoutvision_init,
        "1.0",
        GST_LICENSE_UNKNOWN,
        "GStreamer",
        "http://gstreamer.net/"
)
