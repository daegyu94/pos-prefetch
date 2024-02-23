#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/sync_stream.h>
#include "build/prefetch.grpc.pb.h"

#include "my_types.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

using prefetch::Prefetcher;
using prefetch::PrefetchReply;
using prefetch::PrefetchRequest;

class GRPCHandler {
public:
    GRPCHandler(const std::string& tgt_addr, const std::string& tgt_port);
    ~GRPCHandler();

    void SendMsg(const RpcMessage &rpc_msg);

    void Process(const RpcMessage& msg);

    // Loop while listening for completed responses.
    // Prints out the response from the server.
    void AsyncCompleteRpc();

private:
    // Out of the passed in Channel comes the stub, stored here, our view of the
    // server's exposed services.
    std::unique_ptr<prefetch::Prefetcher::Stub> _stub;

    // struct for keeping state and data information
    struct AsyncClientCall {
        // Container for the data we expect from the server.
        PrefetchReply reply;
        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;
        // Storage for the status of the RPC upon completion.
        Status status;
        std::unique_ptr<ClientAsyncResponseReader<PrefetchReply>> response_reader;
    };

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue _cq;

    std::thread _thd;
};
