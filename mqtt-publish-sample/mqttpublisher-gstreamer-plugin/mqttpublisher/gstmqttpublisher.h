// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef __GST_MQTTPUBLISHER_H__
#define __GST_MQTTPUBLISHER_H__

#include <gst/gst.h>
#include "greengrass-client/GreengrassClient.h"

G_BEGIN_DECLS

#define GST_TYPE_MQTTPUBLISHER \
  (gst_mqtt_publisher_get_type())
#define GST_MQTTPUBLISHER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MQTTPUBLISHER,GstMqttPublisher))
#define GST_MQTTPUBLISHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MQTTPUBLISHER,GstMqttPublisherClass))
#define GST_IS_MQTTPUBLISHER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MQTTPUBLISHER))
#define GST_IS_MQTTPUBLISHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MQTTPUBLISHER))

typedef struct _GstMqttPublisher GstMqttPublisher;
typedef struct _GstMqttPublisherClass GstMqttPublisherClass;

struct _GstMqttPublisher
{
    GstElement element;
    GstPad *sinkpad, *srcpad;
    gchar* publish_topic;
    GreengrassClient* greengrass_client;
};

struct _GstMqttPublisherClass
{
    GstElementClass parent_class;
};

GType gst_mqtt_publisher_get_type(void);

G_END_DECLS

#endif //__GST_MQTTPUBLISHER_H__
