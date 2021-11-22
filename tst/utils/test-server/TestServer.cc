#include <grpcpp/grpcpp.h>
#include <string>
#include "Inference.grpc.pb.h"
#include "TestServer.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using namespace AWS::LookoutVision;

/**
 * Test server implementation
 */
class InferenceServiceImplementation final : public EdgeAgent::Service {

    AWS::LookoutVision::ModelStatus describe_model_status;

    Status DetectAnomalies(ServerContext* context, const DetectAnomaliesRequest* request,
                           DetectAnomaliesResponse* reply) override {
        auto result = reply->mutable_detect_anomaly_result();
        result->set_is_anomalous(1);
        result->set_confidence(0.52559);

        return Status::OK;
    }

    Status StartModel(ServerContext* context, const StartModelRequest* request,
                      StartModelResponse* reply) override {
        reply->set_status(AWS::LookoutVision::ModelStatus::STARTING);

        return Status::OK;
    }

    Status StopModel(ServerContext* context, const StopModelRequest* request,
                     StopModelResponse* reply) override {
        reply->set_status(AWS::LookoutVision::ModelStatus::STOPPING);

        return Status::OK;
    }

    Status DescribeModel(ServerContext* context, const DescribeModelRequest* request,
                         DescribeModelResponse* reply) override {
        auto model_description = reply->mutable_model_description();
        model_description->set_status(describe_model_status);

        return Status::OK;
    }

public:
    void setDescribeModelStatus(std::string model_status) {
        if (model_status == "FAILED") {
            describe_model_status = AWS::LookoutVision::ModelStatus::FAILED;
        } else {
            describe_model_status = AWS::LookoutVision::ModelStatus::RUNNING;
        }
    }

};

TestServer::TestServer() {}

void TestServer::RunServer(std::string server_address, std::string model_status) {
    InferenceServiceImplementation service;
    service.setDescribeModelStatus(model_status);

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which
    // communication with client takes place
    builder.RegisterService(&service);

    // Assembling the server
    test_server = std::unique_ptr<Server>(builder.BuildAndStart());
    std::cout << "Server listening on port: " << server_address << std::endl;

    test_server->Wait();
}

void TestServer::RunServerInBackground(std::string server_address, std::string model_status) {
    server_thread = std::thread(&TestServer::RunServer, this, server_address, model_status);
}

void TestServer::StopServer() {
    test_server->Shutdown();
    server_thread.join();
}
