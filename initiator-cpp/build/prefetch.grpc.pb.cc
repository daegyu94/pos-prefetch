// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: prefetch.proto

#include "prefetch.pb.h"
#include "prefetch.grpc.pb.h"

#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/channel_interface.h>
#include <grpcpp/impl/client_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/rpc_service_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/sync_stream.h>
namespace prefetch {

static const char* Prefetcher_method_names[] = {
  "/prefetch.Prefetcher/PrefetchData",
};

std::unique_ptr< Prefetcher::Stub> Prefetcher::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< Prefetcher::Stub> stub(new Prefetcher::Stub(channel, options));
  return stub;
}

Prefetcher::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_PrefetchData_(Prefetcher_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status Prefetcher::Stub::PrefetchData(::grpc::ClientContext* context, const ::prefetch::PrefetchRequest& request, ::prefetch::PrefetchReply* response) {
  return ::grpc::internal::BlockingUnaryCall< ::prefetch::PrefetchRequest, ::prefetch::PrefetchReply, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_PrefetchData_, context, request, response);
}

void Prefetcher::Stub::async::PrefetchData(::grpc::ClientContext* context, const ::prefetch::PrefetchRequest* request, ::prefetch::PrefetchReply* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::prefetch::PrefetchRequest, ::prefetch::PrefetchReply, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_PrefetchData_, context, request, response, std::move(f));
}

void Prefetcher::Stub::async::PrefetchData(::grpc::ClientContext* context, const ::prefetch::PrefetchRequest* request, ::prefetch::PrefetchReply* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_PrefetchData_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::prefetch::PrefetchReply>* Prefetcher::Stub::PrepareAsyncPrefetchDataRaw(::grpc::ClientContext* context, const ::prefetch::PrefetchRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::prefetch::PrefetchReply, ::prefetch::PrefetchRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_PrefetchData_, context, request);
}

::grpc::ClientAsyncResponseReader< ::prefetch::PrefetchReply>* Prefetcher::Stub::AsyncPrefetchDataRaw(::grpc::ClientContext* context, const ::prefetch::PrefetchRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncPrefetchDataRaw(context, request, cq);
  result->StartCall();
  return result;
}

Prefetcher::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      Prefetcher_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< Prefetcher::Service, ::prefetch::PrefetchRequest, ::prefetch::PrefetchReply, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](Prefetcher::Service* service,
             ::grpc::ServerContext* ctx,
             const ::prefetch::PrefetchRequest* req,
             ::prefetch::PrefetchReply* resp) {
               return service->PrefetchData(ctx, req, resp);
             }, this)));
}

Prefetcher::Service::~Service() {
}

::grpc::Status Prefetcher::Service::PrefetchData(::grpc::ServerContext* context, const ::prefetch::PrefetchRequest* request, ::prefetch::PrefetchReply* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace prefetch

