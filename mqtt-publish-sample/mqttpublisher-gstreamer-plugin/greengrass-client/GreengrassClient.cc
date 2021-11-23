#include <iostream>
#include <aws/crt/Api.h>
#include <aws/greengrass/GreengrassCoreIpcClient.h>
#include "GreengrassClient.h"

using namespace Aws::Crt;
using namespace Aws::Greengrass;

ApiHandle api_handle;

const int GreengrassClient::TIMEOUT_IN_SECONDS = 10;

GreengrassClient::GreengrassClient() {
    event_loop_group = new Io::EventLoopGroup(1);
    socket_resolver = new Io::DefaultHostResolver(*event_loop_group, 64, 30);
    bootstrap = new Io::ClientBootstrap(*event_loop_group, *socket_resolver);
    ipc_lifecycle_handler = new IpcClientLifecycleHandler();
    ipc_client = new GreengrassCoreIpcClient(*bootstrap);
    auto connection_status = ipc_client->Connect(*ipc_lifecycle_handler).get();
    if (!connection_status) {
        std::cerr << "Failed to establish IPC connection: " << connection_status.StatusToString() << std::endl;
        throw std::runtime_error("greengrass IPC connection failed");
    }
}

GreengrassClient::OperationStatus GreengrassClient::PublishToIoTMQTT(std::string topic, std::string payload) {
    String publish_payload(payload.c_str());
    String publish_topic(topic.c_str());

    PublishToIoTCoreRequest request;
    Vector<uint8_t> payload_data({publish_payload.begin(), publish_payload.end()});
    request.SetTopicName(publish_topic);
    request.SetPayload(payload_data);
    request.SetQos(QOS_AT_LEAST_ONCE);

    PublishToIoTCoreOperation operation = ipc_client->NewPublishToIoTCore();
    auto activate = operation.Activate(request, nullptr);
    activate.wait();

    auto response_future = operation.GetResult();
    if (response_future.wait_for(std::chrono::seconds(TIMEOUT_IN_SECONDS)) == std::future_status::timeout) {
        std::cerr << "Operation timed out while waiting for response from Greengrass Core." << std::endl;
        return OperationStatus::FAILED;
    }

    auto response = response_future.get();
    if (response) {
        std::cout << "Successfully published to topic: " << publish_topic << std::endl;
        return OperationStatus::SUCCESSFUL;
    } else {
        // An error occurred.
        std::cout << "Failed to publish to topic: " << publish_topic << std::endl;
        auto error_type = response.GetResultType();
        if (error_type == OPERATION_ERROR) {
            auto *error = response.GetOperationError();
            std::cout << "Operation error: " << error->GetMessage().value() << std::endl;
        } else {
            std::cout << "RPC error: " << response.GetRpcError() << std::endl;
        }
        return OperationStatus::FAILED;
    }
}
