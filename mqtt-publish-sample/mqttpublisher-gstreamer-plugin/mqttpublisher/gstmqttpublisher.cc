/**
 * SECTION:element-mqttpublisher
 *
 * Extracts inference results from frames and publishes to IoT MQTT topic
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 *   gst-launch-1.0
 *     videotestsrc num-buffers=1 pattern="ball"
 *   ! 'video/x-raw, format=RGB, width=1280, height=720'
 *   ! videoconvert
 *   ! lookoutvision server-socket="unix:///tmp/aws.iot.lookoutvision.EdgeAgent.sock" model-component="SampleModel"
 *   ! mqttpublisher publish-topic="lookoutvision/anomalydetection/result"
 *   ! videoconvert
 *   ! jpegenc
 *   ! filesink location=./anomaly.jpg
 * ]|
 * </refsect2>
 */

#include <iostream>
#include <gst/gst.h>
#include "gstmqttpublisher.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionmeta.h"
#include "greengrass-client/GreengrassClient.h"

GST_DEBUG_CATEGORY_STATIC(gst_mqtt_publisher_debug);
#define GST_CAT_DEFAULT gst_mqtt_publisher_debug

enum {
    PROP_0,
    PROP_PUBLISH_TOPIC
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

#define gst_mqtt_publisher_parent_class parent_class
G_DEFINE_TYPE(GstMqttPublisher, gst_mqtt_publisher, GST_TYPE_ELEMENT);

static void gst_mqtt_publisher_set_property(GObject * object, guint prop_id,
                                            const GValue * value, GParamSpec * pspec);
static void gst_mqtt_publisher_get_property(GObject * object, guint prop_id,
                                            GValue * value, GParamSpec * pspec);
static void gst_mqtt_publisher_finalize(GObject *object);

static gboolean gst_mqtt_publisher_sink_event(GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_mqtt_publisher_chain(GstPad * pad, GstObject * parent, GstBuffer * buf);

/* initialize the mqttpublisher class */
static void gst_mqtt_publisher_class_init(GstMqttPublisherClass * klass) {
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *) klass;
    gstelement_class = (GstElementClass *) klass;

    gobject_class->set_property = gst_mqtt_publisher_set_property;
    gobject_class->get_property = gst_mqtt_publisher_get_property;
    gobject_class->finalize = gst_mqtt_publisher_finalize;

    g_object_class_install_property(gobject_class, PROP_PUBLISH_TOPIC,
                                    g_param_spec_string("publish-topic", "Publish Topic", "MQTT topic to publish",
                                                        "lookoutvision/anomalydetection/result",
                                                        G_PARAM_READWRITE));

    gst_element_class_set_details_simple(gstelement_class,
                                         "MqttPublisher",
                                         "GstMqttPublisher",
                                         "MQTT Publisher GStreamer plugin",
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
static void gst_mqtt_publisher_init(GstMqttPublisher * filter) {
    filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_mqtt_publisher_sink_event));
    gst_pad_set_chain_function(filter->sinkpad, GST_DEBUG_FUNCPTR(gst_mqtt_publisher_chain));
    GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
    gst_element_add_pad(GST_ELEMENT (filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS(filter->srcpad);
    gst_element_add_pad(GST_ELEMENT (filter), filter->srcpad);

    // Set default properties
    filter->publish_topic = g_strdup("lookoutvision/anomalydetection/result");
    filter->greengrass_client = new GreengrassClient();
}

static void gst_mqtt_publisher_set_property(GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec) {
    GstMqttPublisher *filter = GST_MQTTPUBLISHER(object);

    switch (prop_id) {
        case PROP_PUBLISH_TOPIC:
            g_free(filter->publish_topic);
            filter->publish_topic = g_strdup(g_value_get_string(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_mqtt_publisher_get_property(GObject * object, guint prop_id, GValue * value, GParamSpec * pspec) {
    GstMqttPublisher *filter = GST_MQTTPUBLISHER(object);

    switch (prop_id) {
        case PROP_PUBLISH_TOPIC:
            g_value_set_string(value, filter->publish_topic);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_mqtt_publisher_finalize(GObject *object) {
    GstMqttPublisher *filter = GST_MQTTPUBLISHER(object);
    if (filter) {
        GST_DEBUG_OBJECT(filter, "finalize");
        g_free(filter->publish_topic);
        filter->publish_topic = NULL;
    }
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* this function handles sink events */
static gboolean gst_mqtt_publisher_sink_event(GstPad * pad, GstObject * parent, GstEvent * event) {
    GST_LOG_OBJECT(GST_MQTTPUBLISHER(parent), "Received %s event: %" GST_PTR_FORMAT, GST_EVENT_TYPE_NAME(event), event);
    return gst_pad_event_default(pad, parent, event);
}

/* chain function */
static GstFlowReturn gst_mqtt_publisher_chain(GstPad * pad, GstObject * parent, GstBuffer * buf) {
    GstMqttPublisher *filter;
    filter = GST_MQTTPUBLISHER(parent);

    GstLookoutVisionMeta* lookoutvision_meta = gst_buffer_get_lookout_vision_meta(buf);
    if (lookoutvision_meta) {
        GstLookoutVisionResult* inference_result = lookoutvision_meta->result;
        if (inference_result && inference_result->result_status == GstLookoutVisionResultStatus::SUCCESSFUL) {
            std::string result_message = "Detect Anomaly Result - Is Anomalous? "
                    + std::to_string(inference_result->is_anomalous) + ", Confidence: "
                    + std::to_string(inference_result->confidence);
            filter->greengrass_client->PublishToIoTMQTT(filter->publish_topic, result_message);
            g_print("Published to MQTT topic %s\n", filter->publish_topic);
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
static gboolean mqttpublisher_init(GstPlugin * mqttpublisher) {
    GST_DEBUG_CATEGORY_INIT(gst_mqtt_publisher_debug, "mqttpublisher", 0, "MQTT Publisher plugin");

    return gst_element_register(mqttpublisher, "mqttpublisher", GST_RANK_NONE, GST_TYPE_MQTTPUBLISHER);
}

#ifndef PACKAGE
#define PACKAGE "mqttpublisher"
#endif

GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        mqttpublisher,
        "MQTT Publisher plugin",
        mqttpublisher_init,
        "1.0",
        GST_LICENSE_UNKNOWN,
        "GStreamer",
        "http://gstreamer.net/"
)
