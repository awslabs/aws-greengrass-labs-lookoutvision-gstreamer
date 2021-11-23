// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifndef __TESTSERVER_H__
#define __TESTSERVER_H__

#include <grpcpp/grpcpp.h>
#include <thread>
#include "Inference.grpc.pb.h"

using grpc::Server;

class TestServer {
public:
    TestServer();
    void RunServer(std::string server_address, std::string model_status);
    void RunServerInBackground(std::string server_address, std::string model_status);
    void StopServer();

private:
    std::unique_ptr<Server> test_server;
    std::thread server_thread;
};

#endif //__TESTSERVER_H__
