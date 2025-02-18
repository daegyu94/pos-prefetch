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

DEFINE_string(addr, "10.0.50.8", "tcp addr for connection");
DEFINE_string(port, "50051", "tcp port for connection");
DEFINE_int32(extent_cache, 1, "Enable or disable extent cache");
DEFINE_int32(extent_aligned_dispatch, 1, "Enable or disable extent-aligned dispatch");
DEFINE_int32(trace_fadvise, 1, "Enable or disable fadvise tracing");

struct config_t config;

void set_default_config() {
    config.addr = FLAGS_addr;
    config.port = FLAGS_port;
    config.extent_cache = FLAGS_extent_cache;
    config.extent_aligned_dispatch = FLAGS_extent_aligned_dispatch;
    config.trace_fadvise = FLAGS_trace_fadvise;
}

void print_config() {
    printf("------------------- Config -------------------\n");
    printf("addr=%s, port=%s\n", config.addr.c_str(), config.port.c_str());
    printf("extent_cache=%d\n", config.extent_cache);
    printf("extent_aligned_dispatch=%d\n", config.extent_aligned_dispatch);
    printf("trace_fadvise=%d\n", config.trace_fadvise);
    printf("----------------------------------------------\n");
}

/* grpc_handler, metadata(map) is global extern variable */
int initialize() {
    std::vector<uint32_t> dev_ids = mntpnt_map.GetDevIds();
    if (dev_ids.size() == 0) {
        printf("[ERROR] The target nvme device is not mounted\n");
        return -1;
    }

    GRPCHandler grpc_handler(config.addr, config.port);
    
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
