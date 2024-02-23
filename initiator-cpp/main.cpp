#include <iostream>
#include <gflags/gflags.h>

#include "config.h"
#include "metadata.h"
#include "grpc_handler.h"
#include "bpf_tracer.h"
#include "event_queue.h"
#include "event_handler.h"
#include "monitor.h"

using namespace std;

DEFINE_uint32(server_port, 50000, "tcp port for connection");

struct config_t config;
void set_default_config() {
    config.server_port = FLAGS_server_port;
}

void print_config() {
    printf("------------------- Config -------------------\n");
    printf("tcp_port=%d\n", 
            config.server_port);
    printf("----------------------------------------------\n");
}

/* grpc_handler, metadata(map) is global extern variable */
int initialize() {
    std::string addr = "10.0.0.56";
    std::string port = "50001";

    std::vector<uint32_t> dev_ids = mntpnt_map.GetDevIds();
    if (dev_ids.size() == 0) {
        printf("[ERROR] The target nvme device is not mounted\n");
        return -1;
    }

    GRPCHandler grpc_handler(addr, port);
    
    Translator translator(grpc_handler);

    EventQueue event_queue;

    /* due to staic variable */
    BPFTracer::_event_queue = &event_queue;
    BPFTracer bpf_tracer;

    for (const auto &e : dev_ids) {
        bpf_tracer.UpdateMntMap(e);
    }

    Monitor monitor(event_queue, translator);
    
    EventHandler event_handler(event_queue, translator);

    /* XXX */
    bpf_tracer.Wait();
    event_handler.Wait();

    return 0;
}

int main(int argc, char **argv) {
    google::SetUsageMessage("some usage message");
    google::ParseCommandLineFlags(&argc, &argv, true);

    set_default_config();
    print_config();
    
    if (initialize()) {
        printf("[ERROR] Failed to initialized program\n");
        return -1;
    }

    google::ShutDownCommandLineFlags();

    return 0;
}
