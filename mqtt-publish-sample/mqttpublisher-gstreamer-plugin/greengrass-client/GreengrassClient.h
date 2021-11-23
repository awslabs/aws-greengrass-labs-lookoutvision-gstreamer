// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef __GREENGRASS_CLIENT_H__
#define __GREENGRASS_CLIENT_H__

#include <aws/crt/Api.h>
#include <aws/greengrass/GreengrassCoreIpcClient.h>

using namespace Aws::Crt;
using namespace Aws::Greengrass;

class IpcClientLifecycleHandler : public ConnectionLifecycleHandler {
    void OnConnectCallback() override {
        std::cout << "OnConnectCallback" << std::endl;
    }

    void OnDisconnectCallback(RpcError error) override {
        std::cout << "OnDisconnectCallback: " << error.StatusToString() << std::endl;
    }

    bool OnErrorCallback(RpcError error) override {
        std::cout << "OnErrorCallback: " << error.StatusToString() << std::endl;
        return true;
    }
};

class GreengrassClient{
public:
    typedef enum _OperationStatus {
        SUCCESSFUL = 0,
        FAILED = -1
    } OperationStatus;

    GreengrassClient();
    OperationStatus PublishToIoTMQTT(std::string topic, std::string payload);

private:
    static const int TIMEOUT_IN_SECONDS;
    Io::EventLoopGroup* event_loop_group;
    Io::DefaultHostResolver* socket_resolver;
    Io::ClientBootstrap* bootstrap;
    IpcClientLifecycleHandler* ipc_lifecycle_handler;
    GreengrassCoreIpcClient* ipc_client;
};

#endif //__GREENGRASS_CLIENT_H__
