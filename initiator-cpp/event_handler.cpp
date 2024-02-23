#include "event_handler.h"

#include "stat.h"

#define DMFP_EH_INFO
#ifdef DMFP_EH_INFO
#define dmfp_eh_info(str, ...) \
    printf("[INFO] EventHandler::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_eh_info(str, ...) do {} while (0)
#endif

//#define DMFP_EH_DEBUG
#ifdef DMFP_EH_DEBUG
#define dmfp_eh_debug(str, ...) \
    printf("[DEBUG] EventHandler::%s: " str, __func__, ##__VA_ARGS__)
#else
#define dmfp_eh_debug(str, ...) do {} while (0)
#endif

EventHandler::EventHandler(EventQueue &queue, Translator &translator) : 
    _event_queue(queue), _translator(translator), is_running(true) {
        dmfp_eh_info("\n");
        Start();
    }

void EventHandler::Start() {
    thd = std::thread(&EventHandler::HandleEvents, this);
}

EventHandler::~EventHandler() {
    Stop();
}

void EventHandler::Wait() {
    if (thd.joinable()) {
        thd.join();
    }
}

void EventHandler::Stop() {
    is_running = false;
    _event_queue.Stop();
    if (thd.joinable()) {
        thd.join();
    }
}

static uint64_t mono_counter = 0;

void EventHandler::ProcessEvent(void *item) {
    BPFEvent *event = static_cast<BPFEvent *>(item);

    dmfp_eh_debug("event=%d\n", event->type);
    if (event->type == EVENT_PAGE_DELETION) {
        _translator.ProcessPageDeletion(event);
        counter.event_page_deletion++;
    } else if (event->type == EVENT_PAGE_REFERENCED) {
        _translator.ProcessPageReference(event);
        counter.event_page_referenced++;
    } else if (event->type == EVENT_EXT4_MPAGE_READPAGES) {
        _translator.ProcessReadpages(event);
        counter.event_readpages++;
    } else if (event->type == EVENT_VFS_UNLINK) {
        _translator.ProcessUnlink(event);
        counter.event_vfs_unlink++;
    } else if (event->type == EVENT_CLEANCACHE_REPL) {
        counter.event_cleancache_repl++;
    } else {
        printf("[ERROR] Unknown event_type=%u\n", event->type); 
    }
    
    if ((mono_counter++ % 32) == 0) {
        counter.event_queue_size = _event_queue.Size();
    }

    delete event;
}

void EventHandler::HandleEvents() {
    dmfp_eh_info("\n");
    
    while (is_running) {
        void *item = _event_queue.Dequeue();
        if (item != nullptr) {
            ProcessEvent(item);
        }
    }
}


