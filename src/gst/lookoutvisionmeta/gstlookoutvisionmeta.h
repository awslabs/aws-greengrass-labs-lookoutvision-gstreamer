// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef __GST_LOOKOUTVISION_META_H__
#define __GST_LOOKOUTVISION_META_H__

#include <gst/gst.h>
#include "gstlookoutvisionresult.h"

G_BEGIN_DECLS

typedef struct _GstLookoutVisionMeta {
    GstMeta meta;

    GstLookoutVisionResult* result;
} GstLookoutVisionMeta;

#define GST_LOOKOUT_VISION_META_NAME "GstLookoutVisionMeta"
#define GST_LOOKOUT_VISION_META_API_NAME "GstLookoutVisionMetaAPI"

#define GST_LOOKOUT_VISION_META_API_TYPE (gst_lookout_vision_meta_api_get_type())
#define GST_LOOKOUT_VISION_META_INFO (gst_lookout_vision_meta_get_info())

GST_EXPORT
        GType gst_lookout_vision_meta_api_get_type (void);
GST_EXPORT
        const GstMetaInfo* gst_lookout_vision_meta_get_info (void);

GST_EXPORT
        GstLookoutVisionMeta* gst_buffer_add_lookout_vision_meta (GstBuffer *buffer, GstLookoutVisionResult* result);

GST_EXPORT
        GstLookoutVisionMeta* gst_buffer_get_lookout_vision_meta (GstBuffer *buffer);

G_END_DECLS

#endif /* __GST_LOOKOUTVISION_META_H__ */
