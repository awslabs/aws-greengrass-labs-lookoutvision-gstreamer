#ifndef __GST_LOOKOUTVISION_RESULT_H__
#define __GST_LOOKOUTVISION_RESULT_H__

#include <gst/gst.h>
#include <string>

G_BEGIN_DECLS

typedef enum _GstLookoutVisionResultStatus {
    SUCCESSFUL,
    FAILED
} GstLookoutVisionResultStatus;

typedef struct _GstLookoutVisionResult {
    bool is_anomalous;
    float confidence;
    GstLookoutVisionResultStatus result_status;
    std::string error_message;
} GstLookoutVisionResult;

G_END_DECLS

#endif //__GST_LOOKOUTVISION_RESULT_H__
