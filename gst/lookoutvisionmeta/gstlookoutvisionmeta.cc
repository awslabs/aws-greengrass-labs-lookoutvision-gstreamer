#include <gst/gst.h>
#include "gstlookoutvisionmeta.h"

static gboolean gst_lookout_vision_meta_init(GstLookoutVisionMeta *meta, gpointer params, GstBuffer *buf) {
    meta->result = NULL;

    return TRUE;
}

static void gst_lookout_vision_meta_free(GstLookoutVisionMeta *meta, GstBuffer *buf) {
    delete meta->result;
    meta->result = NULL;
}

static gboolean gst_lookout_vision_meta_transform(GstBuffer *dest, GstMeta *meta, GstBuffer *buffer, GQuark type,
                                                  gpointer data) {
    GstLookoutVisionMeta *dest_meta;
    GstLookoutVisionMeta *src_meta = (GstLookoutVisionMeta*) meta;

    dest_meta = gst_buffer_add_lookout_vision_meta(dest, src_meta->result);

    return (dest_meta != NULL);
}

GType gst_lookout_vision_meta_api_get_type(void) {
    static volatile GType type = 0;
    static const gchar *tags[] = { NULL };

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register(GST_LOOKOUT_VISION_META_API_NAME, tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

const GstMetaInfo* gst_lookout_vision_meta_get_info(void) {
    static const GstMetaInfo *info = NULL;

    if (g_once_init_enter(&info)) {
        const GstMetaInfo *meta = gst_meta_register(GST_LOOKOUT_VISION_META_API_TYPE,
                                                    GST_LOOKOUT_VISION_META_NAME, sizeof(GstLookoutVisionMeta),
                                                    (GstMetaInitFunction) gst_lookout_vision_meta_init,
                                                    (GstMetaFreeFunction) gst_lookout_vision_meta_free,
                                                    (GstMetaTransformFunction) gst_lookout_vision_meta_transform);
        g_once_init_leave(&info, meta);
    }
    return info;
}

GstLookoutVisionMeta* gst_buffer_add_lookout_vision_meta(GstBuffer *buffer, GstLookoutVisionResult* result) {
    GstLookoutVisionMeta *meta;

    g_return_val_if_fail(buffer, NULL);

    meta = (GstLookoutVisionMeta *) gst_buffer_add_meta(buffer, GST_LOOKOUT_VISION_META_INFO, NULL);

    if (!meta)
        return NULL;

    meta->result = new GstLookoutVisionResult();
    *(meta->result) = *result;

    return meta;
}

GstLookoutVisionMeta* gst_buffer_get_lookout_vision_meta(GstBuffer *buffer) {
    GstLookoutVisionMeta *meta;
    const GstMetaInfo *info = gst_meta_get_info(GST_LOOKOUT_VISION_META_NAME);

    g_return_val_if_fail(buffer, NULL);

    meta = (GstLookoutVisionMeta*) gst_buffer_get_meta(buffer, info->api);

    return meta;
}
