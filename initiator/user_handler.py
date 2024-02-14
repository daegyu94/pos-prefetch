import threading
import time
import queue 
import asyncio

from stats import *
from translator import *
from steering import *
from my_types import *

grpc_handler_on = False
hotness_on = False 

class UserHandler(threading.Thread):
    def __init__(self, event_queue, meta_dict, translator, grpc_handler, steering):
        threading.Thread.__init__(self)
        super().__init__()
        self.name = "UserHandler"

        self._event_queue = event_queue
        self._meta_dict = meta_dict
        self._translator = translator
        self._grpc_handler = grpc_handler
        self._steering = Steering() if steering == 1 else None
    
    async def handle_page_deletion(self, event):
        if self._steering:
            event_list = self._steering.process(event)
        else:
            event_list = event 
        
        for event in event_list:
            subsys_id, ns_id = self._meta_dict.get_prefetch_meta(event.dev_id)
            if subsys_id is None or ns_id is None: 
                """ rarely, nvme is not mounted """
                continue
            pba_list = self._translator.trans_pba(event)
            """ TODO: send meta info (start_pba, prefetch length) """
            for pba, _ in pba_list:
                if pba is None: 
                    """ not found in fast path, deleted or not tracked """
                    Stats.trans_pba_failed += 1
                    continue
                Stats.trans_pba_fast += 1
                if grpc_handler_on:
                    await self._grpc_handler.handle(TransInfo(subsys_id, ns_id, pba))

    async def handle(self):
        while True:
            try:
                event = self._event_queue.get(block=True, timeout=1)
            
                if isinstance(event, ReadPagesInfo):
                    self._translator.handle_readpages(event)

                elif isinstance(event, PageInfo):
                    #if event.event_type == EventType.EVENT_PAGE_REFERENCED:
                    #    self._translator.handle_page_reference(event)
                    #elif event.event_type == EventType.EVENT_VFS_UNLINK:
                    if event.event_type == EventType.EVENT_VFS_UNLINK:
                        self._translator.handle_vfs_unlink(event)
                    else:
                        print("[ERROR] Unknown page event type={}" .format(event.event_type))

                elif isinstance(event, list):
                    if event[0].event_type == EventType.EVENT_PAGE_REFERENCED:
                        #start_time = time.time_ns()
                        for ev in event:
                            self._translator.handle_page_reference(ev)
                        #end_time = time.time_ns()
                        #elapsed_time_microseconds = (end_time - start_time) / 1000
                        #print(f"handle_page_reference: {elapsed_time_microseconds:.2f} us")
                    else:
                        """ case for page deletion """
                        await self.handle_page_deletion(event)
                else:
                    print("[ERROR] Unknown event type={}" .format(type(event)))

                self._event_queue.task_done()
                Stats.qsize = self._event_queue.qsize()
            except queue.Empty:
                pass

    def run(self):
        asyncio.run(self.handle())
        
