#include "grpc_handler.h"

#include "stat.h"

#define DMFP_GRPC_INFO
#ifdef DMFP_GRPC_INFO
#define dmfp_grpc_info(str, ...) \
    printf("[INFO] GRPCHandler::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_grpc_info(str, ...) do {} while (0)
#endif

//#define DMFP_GRPC_DEBUG
#ifdef DMFP_GRPC_DEBUG
#define dmfp_grpc_debug(str, ...) \
    printf("[DEBUG] GRPCHandler::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_grpc_debug(str, ...) do {} while (0)
#endif

#define MAX_GRPC_NUM_MSGS 256

GRPCHandler::GRPCHandler(const std::string& tgt_addr, const std::string& tgt_port) {
    dmfp_grpc_info("\n");
    std::string tgt_str = tgt_addr + ":" + tgt_port;
    _stub = Prefetcher::NewStub(grpc::CreateChannel(tgt_str, grpc::InsecureChannelCredentials()));
    _thd = std::thread(&GRPCHandler::AsyncCompleteRpc, this);
}

GRPCHandler::~GRPCHandler() {
    if (_thd.joinable()) {
        _thd.join();
    }
}

#if 0
void GRPCHandler::SendMsg(const RpcMessage &rpc_msg) {
	prefetch::PrefetchRequest request;

	for (int i = 0; i < 1; i++ ) {
		prefetch::PrefetchMsg* msg = request.add_msgs();
		uint32_t subsys_id = 1;
		uint32_t ns_id = 2;
		uint64_t pba = 3;
		uint32_t length = 4;

		msg->set_subsys_id(subsys_id);
		msg->set_ns_id(ns_id);
		msg->set_pba(pba);
		//msg->set_length(length);
	}

	// Call object to store rpc data
	AsyncClientCall *call = new AsyncClientCall;

	// stub_->PrepareAsyncSayHello() creates an RPC object, returning
	// an instance to store in "call" but does not actually start the RPC
	// Because we are using the asynchronous API, we need to hold on to
	// the "call" instance in order to get updates on the ongoing RPC.
	call->response_reader =
		_stub->PrepareAsyncPrefetchData(&call->context, request, &_cq);

	// StartCall initiates the RPC call
	call->response_reader->StartCall();

	// Request that, upon completion of the RPC, "reply" be updated with the
	// server's response; "status" with the indication of whether the operation
	// was successful. Tag the request with the memory address of the call
	// object.
	call->response_reader->Finish(&call->reply, &call->status, (void *) call);

    counter.grpc_num_send_msgs++;
}
#else 
void GRPCHandler::SendMsg(const RpcMessage &rpc_msg) {
    br_declare_ts(all);

    if (num_msgs_ == 0) {
        requests_ = prefetch::PrefetchRequest{};    
    }
    
    if (num_msgs_ < MAX_GRPC_NUM_MSGS) {
		prefetch::PrefetchMsg* msg = requests_.add_msgs();

		msg->set_subsys_id(rpc_msg.subsys_id);
		msg->set_ns_id(rpc_msg.ns_id);
		msg->set_pba(rpc_msg.pba);
       
        num_msgs_++;

        // 아직 최대 개수가 안 찼으면 종료
        if (num_msgs_ < MAX_GRPC_NUM_MSGS) {
            return;
        }
	}
    
    br_start_ts(all);
	// Call object to store rpc data
	AsyncClientCall *call = new AsyncClientCall;

	// stub_->PrepareAsyncSayHello() creates an RPC object, returning
	// an instance to store in "call" but does not actually start the RPC
	// Because we are using the asynchronous API, we need to hold on to
	// the "call" instance in order to get updates on the ongoing RPC.
	call->response_reader =
		_stub->PrepareAsyncPrefetchData(&call->context, requests_, &_cq);

	// StartCall initiates the RPC call
	call->response_reader->StartCall();

	// Request that, upon completion of the RPC, "reply" be updated with the
	// server's response; "status" with the indication of whether the operation
	// was successful. Tag the request with the memory address of the call
	// object.
	call->response_reader->Finish(&call->reply, &call->status, (void *) call);
    br_end_ts(all, BR_RPC);

    counter.grpc_num_send_msgs += MAX_GRPC_NUM_MSGS;
    
    num_msgs_ = 0;
}
#endif 


void GRPCHandler::Process(const RpcMessage& msg) {
    dmfp_grpc_debug("subsys_id=%u, ns_id=%u, pba=%lu, length=%u\n", 
            msg.subsys_id, msg.ns_id, msg.pba, msg.length);
    SendMsg(msg);
}

void GRPCHandler::AsyncCompleteRpc() {
	void* got_tag;
	bool ok = false;
    
    dmfp_grpc_info("\n");

	// Block until the next result is available in the completion queue "cq".
	while (_cq.Next(&got_tag, &ok)) {
		// The tag in this example is the memory location of the call object
		AsyncClientCall* call = static_cast<AsyncClientCall*>(got_tag);

		// Verify that the request was completed successfully. Note that "ok"
		// corresponds solely to the request for updates introduced by Finish().
		GPR_ASSERT(ok);
#if 0
		if (call->status.ok())
			//std::cout << "received: " << std::endl;
		    //std::cout << "received: " << call->reply.success() << std::endl;
		else
			std::cout << "RPC failed" << std::endl;
#endif
		// Once we're complete, deallocate the call object.
		delete call;
	}
}
