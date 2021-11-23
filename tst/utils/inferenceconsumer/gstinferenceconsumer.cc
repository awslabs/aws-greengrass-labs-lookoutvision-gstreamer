// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <gst/gst.h>
#include "gstinferenceconsumer.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionmeta.h"

GST_DEBUG_CATEGORY_STATIC(gst_inference_consumer_debug);
#define GST_CAT_DEFAULT gst_inference_consumer_debug

enum {
    PROP_0
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

#define gst_inference_consumer_parent_class parent_class
G_DEFINE_TYPE(GstInferenceConsumer, gst_inference_consumer, GST_TYPE_ELEMENT);

static void gst_inference_consumer_set_property(GObject * object, guint prop_id,
                                            const GValue * value, GParamSpec * pspec);
static void gst_inference_consumer_get_property(GObject * object, guint prop_id,
                                            GValue * value, GParamSpec * pspec);
static void gst_inference_consumer_finalize(GObject *object);

static gboolean gst_inference_consumer_sink_event(GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_inference_consumer_chain(GstPad * pad, GstObject * parent, GstBuffer * buf);

/* initialize the inferenceconsumer class */
static void gst_inference_consumer_class_init(GstInferenceConsumerClass * klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_inference_consumer_set_property;
    gobject_class->get_property = gst_inference_consumer_get_property;
    gobject_class->finalize = gst_inference_consumer_finalize;

    gst_element_class_set_details_simple(gstelement_class,
                                         "InferenceConsumer",
                                         "GstInferenceConsumer",
                                         "Inference Consumer GStreamer plugin",
                                         "Amazon");

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}

/*
 * initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_inference_consumer_init(GstInferenceConsumer * filter) {
    filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_inference_consumer_sink_event));
    gst_pad_set_chain_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_inference_consumer_chain));
    GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
    gst_element_add_pad(GST_ELEMENT (filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS(filter->srcpad);
    gst_element_add_pad(GST_ELEMENT (filter), filter->srcpad);
}

static void gst_inference_consumer_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
}

static void gst_inference_consumer_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
}

static void gst_inference_consumer_finalize(GObject *object) {
    GstInferenceConsumer *filter = GST_INFERENCECONSUMER(object);
    if (filter) {
        GST_DEBUG_OBJECT(filter, "finalize");
    }
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* this function handles sink events */
static gboolean gst_inference_consumer_sink_event(GstPad * pad, GstObject * parent, GstEvent * event) {
    GST_LOG_OBJECT(GST_INFERENCECONSUMER(parent), "Received %s event: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(event), event);
    return gst_pad_event_default(pad, parent, event);
}

/* chain function */
static GstFlowReturn gst_inference_consumer_chain(GstPad * pad, GstObject * parent, GstBuffer * buf) {
    GstInferenceConsumer *filter;
    filter = GST_INFERENCECONSUMER(parent);

    GstLookoutVisionMeta* lookoutvision_meta = gst_buffer_get_lookout_vision_meta(buf);
    if (lookoutvision_meta) {
        GstLookoutVisionResult* inference_result = lookoutvision_meta->result;
        if (inference_result && inference_result->result_status == GstLookoutVisionResultStatus::SUCCESSFUL) {
            std::string result_message = "Detect Anomaly Result - Is Anomalous? "
                                         + std::to_string(inference_result->is_anomalous) + ", Confidence: "
                                         + std::to_string(inference_result->confidence);
            std::cout << result_message << std::endl;
        } else {
            std::cout << "Inference call failed / No result in metadata" << std::endl;
        }
    } else {
        std::cout << "Failed to read metadata" << std::endl;
    }

    return gst_pad_push(filter->srcpad, buf);
}

/*
 * entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean inferenceconsumer_init(GstPlugin * inferenceconsumer) {
    GST_DEBUG_CATEGORY_INIT(gst_inference_consumer_debug, "inferenceconsumer", 0, "Inference Consumer plugin");

    return gst_element_register(inferenceconsumer, "inferenceconsumer", GST_RANK_NONE, GST_TYPE_INFERENCECONSUMER);
}

#ifndef PACKAGE
#define PACKAGE "inferenceconsumer"
#endif

GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        inferenceconsumer,
        "Inference Consumer plugin",
        inferenceconsumer_init,
        "1.0",
        GST_LICENSE_UNKNOWN,
        "GStreamer",
        "http://gstreamer.net/"
)
