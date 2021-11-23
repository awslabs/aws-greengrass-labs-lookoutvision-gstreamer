// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef __GST_LOOKOUTVISION_H__
#define __GST_LOOKOUTVISION_H__

#include <gst/gst.h>
#include "lookoutvision-client/LookoutVisionInferenceClient.h"

G_BEGIN_DECLS

#define GST_TYPE_LOOKOUTVISION \
  (gst_lookout_vision_get_type())
#define GST_LOOKOUTVISION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_LOOKOUTVISION,GstLookoutVision))
#define GST_LOOKOUTVISION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_LOOKOUTVISION,GstLookoutVisionClass))
#define GST_IS_LOOKOUTVISION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_LOOKOUTVISION))
#define GST_IS_LOOKOUTVISION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_LOOKOUTVISION))

typedef struct _GstLookoutVision GstLookoutVision;
typedef struct _GstLookoutVisionClass GstLookoutVisionClass;

struct _GstLookoutVision {
    GstElement element;
    GstPad *sinkpad, *srcpad;
    LookoutVisionInferenceClient *inference_client;
    int width;
    int height;
    gchar* server_socket;
    gchar* model_component;
    guint model_status_timeout;
};

struct _GstLookoutVisionClass {
    GstElementClass parent_class;
};

GType gst_lookout_vision_get_type(void);

G_END_DECLS

#endif /* __GST_LOOKOUTVISION_H__ */
