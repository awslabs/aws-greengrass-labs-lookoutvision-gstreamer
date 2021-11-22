#include <gst/gst.h>
#include <gtest/gtest.h>
#include "fff.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionresult.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionmeta.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(GstMeta*, gst_buffer_add_meta, GstBuffer*, const GstMetaInfo*, gpointer);
FAKE_VALUE_FUNC(GstMeta*, gst_buffer_get_meta, GstBuffer*, GType);

class gstlookoutvisionmetatest : public testing::Test
{
public:
    void SetUp()
    {
        RESET_FAKE(gst_buffer_add_meta);
        RESET_FAKE(gst_buffer_get_meta);
        FFF_RESET_HISTORY();
    }
};

TEST(gstlookoutvisionmetatest, gst_buffer_add_meta_returns_null_test) {
    GstMeta* return_vals[1] = {NULL};
    SET_RETURN_SEQ(gst_buffer_add_meta, return_vals, 1);

    GstLookoutVisionResult* inference_result = new GstLookoutVisionResult{0, 0, GstLookoutVisionResultStatus::FAILED,
                                                                          "inference failed"};
    guint8* data = new guint8[120]{};
    GstBuffer* buffer = gst_buffer_new_wrapped(data, 120);
    GstLookoutVisionMeta* meta = gst_buffer_add_lookout_vision_meta(buffer, inference_result);
    ASSERT_EQ(meta, nullptr);
}

TEST(gstlookoutvisionmetatest, gst_buffer_get_meta_returns_null_test) {
    GstMeta* return_vals[1] = {NULL};
    SET_RETURN_SEQ(gst_buffer_get_meta, return_vals, 1);

    guint8* data = new guint8[120]{};
    GstBuffer* buffer = gst_buffer_new_wrapped(data, 120);
    GstLookoutVisionMeta* meta = gst_buffer_get_lookout_vision_meta(buffer);
    ASSERT_EQ(meta, nullptr);
}

TEST(gstlookoutvisionmetatest, null_buffer_test) {
    GstLookoutVisionResult* inference_result = new GstLookoutVisionResult{0, 0, GstLookoutVisionResultStatus::FAILED,
                                                                          "inference failed"};

    GstLookoutVisionMeta* meta_added = gst_buffer_add_lookout_vision_meta(NULL, inference_result);
    ASSERT_EQ(meta_added, nullptr);

    GstLookoutVisionMeta* meta_retrieved = gst_buffer_get_lookout_vision_meta(NULL);
    ASSERT_EQ(meta_retrieved, nullptr);
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    testing::InitGoogleTest();
    RUN_ALL_TESTS();

    return 0;
}
