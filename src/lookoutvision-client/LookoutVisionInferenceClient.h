#ifndef __LOOKOUTVISION_INFERENCE_CLIENT_H__
#define __LOOKOUTVISION_INFERENCE_CLIENT_H__

#include <glib.h>
#include "Inference.grpc.pb.h"
#include "gst/lookoutvisionmeta/gstlookoutvisionresult.h"

class LookoutVisionInferenceClient{
public:
    typedef enum _OperationStatus {
        SUCCESSFUL = 0,
        FAILED = -1
    } OperationStatus;

    LookoutVisionInferenceClient(std::string server_socket);
    LookoutVisionInferenceClient(AWS::LookoutVision::EdgeAgent::StubInterface* inference_stub);
    ~LookoutVisionInferenceClient();
    void setServerSocket(std::string server_socket);
    GstLookoutVisionResult* DetectAnomalies(std::string model_component, guint8* frame, size_t bytes_size, size_t width,
                                            size_t height);
    OperationStatus StartModel(std::string model_component, int model_status_timeout);

private:
    static const int POLLING_INTERVAL_IN_SECONDS;
    #ifdef SHARED_MEMORY
    static const std::string SHM_NAME;
    size_t shm_size = 1920*1920*3;
    int shm_fd;
    uint8_t* shm_data;
    #endif
    std::string server_socket;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<AWS::LookoutVision::EdgeAgent::StubInterface> stub;

    #ifdef SHARED_MEMORY
    void writeSHM(guint8* buf, size_t bytes_size);
    void setupSHM();
    #endif
    bool waitForModelStatusWithTimeout(std::string model_component,
                                       AWS::LookoutVision::ModelStatus expected_status, int timeout_in_seconds);
    AWS::LookoutVision::ModelStatus* getModelStatus(std::string model_component);
};

#endif
