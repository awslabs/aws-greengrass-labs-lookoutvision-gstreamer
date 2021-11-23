// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef __GST_INFERENCECONSUMER_H__
#define __GST_INFERENCECONSUMER_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_INFERENCECONSUMER \
  (gst_inference_consumer_get_type())
#define GST_INFERENCECONSUMER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_INFERENCECONSUMER,GstInferenceConsumer))
#define GST_INFERENCECONSUMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_INFERENCECONSUMER,GstInferenceConsumerClass))
#define GST_IS_INFERENCECONSUMER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_INFERENCECONSUMER))
#define GST_IS_INFERENCECONSUMER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_INFERENCECONSUMER))

typedef struct _GstInferenceConsumer GstInferenceConsumer;
typedef struct _GstInferenceConsumerClass GstInferenceConsumerClass;

struct _GstInferenceConsumer
{
    GstElement element;
    GstPad *sinkpad, *srcpad;
};

struct _GstInferenceConsumerClass
{
    GstElementClass parent_class;
};

GType gst_inference_consumer_get_type(void);

G_END_DECLS

#endif //__GST_INFERENCECONSUMER_H__
