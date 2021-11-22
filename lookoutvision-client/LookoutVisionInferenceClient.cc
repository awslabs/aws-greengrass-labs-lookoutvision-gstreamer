#include <grpcpp/grpcpp.h>
#include <glib.h>
#include <string>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "Inference.grpc.pb.h"
#include "LookoutVisionInferenceClient.h"

const int LookoutVisionInferenceClient::POLLING_INTERVAL_IN_SECONDS = 5;
#ifdef SHARED_MEMORY
const std::string LookoutVisionInferenceClient::SHM_NAME = "/gstreamer-lookoutvision-bitmap";
#endif

LookoutVisionInferenceClient::LookoutVisionInferenceClient(std::string server_url) {
    setServerUrl(server_url);
    #ifdef SHARED_MEMORY
    shm_fd = shm_open(SHM_NAME.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        throw std::runtime_error("shm_open failed");
    }
    ftruncate(shm_fd, shm_size);
    shm_data = (uint8_t*) mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    #endif
}

LookoutVisionInferenceClient::~LookoutVisionInferenceClient() {
    stub.reset();
    channel.reset();
    #ifdef SHARED_MEMORY
    munmap(shm_data, shm_size);
    close(shm_fd);
    shm_unlink(SHM_NAME.c_str());
    #endif
}

void LookoutVisionInferenceClient::setServerUrl(std::string server_url) {
    this->server_url = server_url;
    channel = grpc::CreateChannel(this->server_url, grpc::InsecureChannelCredentials());
    stub = AWS::LookoutVision::EdgeAgent::NewStub(channel);
}

GstLookoutVisionResult* LookoutVisionInferenceClient::DetectAnomalies(std::string model_component, guint8* buf,
                                                                      size_t bytes_size, size_t width, size_t height) {
    AWS::LookoutVision::DetectAnomaliesRequest request;
    AWS::LookoutVision::DetectAnomaliesResponse reply;
    grpc::ClientContext context;

    try {
        request.set_model_component(model_component);
        auto bitmap = request.mutable_bitmap();
        bitmap->set_width(width);
        bitmap->set_height(height);

        #ifdef SHARED_MEMORY
        writeSHM(buf, bytes_size);
        auto shared_memory_handle = bitmap->mutable_shared_memory_handle();
        shared_memory_handle->set_size(bytes_size);
        shared_memory_handle->set_offset(0);
        shared_memory_handle->set_name(SHM_NAME);
        #else
        bitmap->set_byte_data(buf, bytes_size);
        #endif

        grpc::Status status = stub->DetectAnomalies(&context, request, &reply);

        if (status.ok()) {
            return new GstLookoutVisionResult{reply.detect_anomaly_result().is_anomalous(),
                                              reply.detect_anomaly_result().confidence(),
                                              GstLookoutVisionResultStatus::SUCCESSFUL, ""};
        }
        else {
            std::cout << "DetectAnomalies failed with error "
            << status.error_code() << ": " << status.error_message() << std::endl;
            return new GstLookoutVisionResult{0, 0, GstLookoutVisionResultStatus::FAILED,
                                              status.error_code() + ": " + status.error_message()};
        }
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return new GstLookoutVisionResult{0, 0, GstLookoutVisionResultStatus::FAILED, e.what()};
    }
}

#ifdef SHARED_MEMORY
void LookoutVisionInferenceClient::writeSHM(guint8* buf, size_t bytes_size) {
    if (bytes_size > shm_size) {
        shm_size = bytes_size;
        ftruncate(shm_fd, shm_size);
        shm_data = (uint8_t*) mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    }
    memcpy(shm_data, buf, bytes_size);
}
#endif

LookoutVisionInferenceClient::OperationStatus LookoutVisionInferenceClient::StartModel(std::string model_component,
                                                                                       int model_status_timeout) {
    AWS::LookoutVision::StartModelRequest request;
    AWS::LookoutVision::StartModelResponse reply;
    grpc::ClientContext context;

    try {
        request.set_model_component(model_component);
        grpc::Status status = stub->StartModel(&context, request, &reply);

        if (!status.ok()) {
            std::cout << "StartModel returned error "
            << status.error_code() << ": " << status.error_message() << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    if (waitForModelStatusWithTimeout(model_component, AWS::LookoutVision::ModelStatus::RUNNING,
                                      model_status_timeout)) {
        return OperationStatus::SUCCESSFUL;
    } else {
        std::cout << "Model didn't reach RUNNING state within " << model_status_timeout << " seconds" << std::endl;
    }

    return OperationStatus::FAILED;
}

bool LookoutVisionInferenceClient::waitForModelStatusWithTimeout(std::string model_component,
                                                                 AWS::LookoutVision::ModelStatus expected_status,
                                                                 int timeout_in_seconds) {
    std::time_t start_time = std::time(NULL);

    while (std::time(NULL) - start_time < timeout_in_seconds) {
        AWS::LookoutVision::ModelStatus* model_status = getModelStatus(model_component);
        if (model_status && *model_status == expected_status) {
            delete model_status;
            return true;
        }
        delete model_status;
        usleep(POLLING_INTERVAL_IN_SECONDS * 1000000);
    }
    return false;
}

AWS::LookoutVision::ModelStatus* LookoutVisionInferenceClient::getModelStatus(std::string model_component) {
    AWS::LookoutVision::DescribeModelRequest request;
    AWS::LookoutVision::DescribeModelResponse reply;
    grpc::ClientContext context;
    AWS::LookoutVision::ModelStatus* model_status = NULL;

    try {
        request.set_model_component(model_component);
        grpc::Status status = stub->DescribeModel(&context, request, &reply);

        if (status.ok()) {
            model_status = new AWS::LookoutVision::ModelStatus{reply.model_description().status()};
        } else {
            std::cout << "DescribeModel failed with error "
            << status.error_code() << ": " << status.error_message() << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return model_status;
}
