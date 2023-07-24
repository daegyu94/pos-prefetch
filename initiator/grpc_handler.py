"""The Python AsyncIO implementation of the GRPC helloworld.Greeter client."""

import logging
import asyncio
import grpc
import prefetch_pb2
import prefetch_pb2_grpc
import threading
import time

from metadata import *
from translator import *
from stats import *
from steering import *
from config import *

steering_on = False

# TODO: process buffered msg with timeout
class PrefetchClient:
    def __init__(self, host, port):
        self._channel = grpc.aio.insecure_channel(f"{host}:{port}")
        self._stub = prefetch_pb2_grpc.PrefetcherStub(self._channel)
        self._request = None
        self._num_msgs = 0

    async def __send_msgs(self):
        Stats.send_msgs += self._num_msgs
        response = await self._stub.PrefetchData(self._request)
        self._num_msgs = 0
        self._request = None

    async def send_msgs(self, trans_info):
        if self._num_msgs == 0:
            self._request = prefetch_pb2.PrefetchRequest()

        msg = prefetch_pb2.PrefetchMsg(
                subsys_id = trans_info.subsys_id,
                ns_id = trans_info.ns_id,
                pba = trans_info.pba
                )

        self._request.msgs.append(msg)
        self._num_msgs += 1
        if self._num_msgs == ConstConfig.MAX_GRPC_NUM_MSGS:
            await self.__send_msgs()

    async def __aenter__(self):
        return self
        
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self._channel.close()


class gRPCHandler(threading.Thread):
    def __init__(self, meta_dict, translator, tgt_addr, tgt_port, steering):
        threading.Thread.__init__(self)
        super().__init__()
        self.name = "gRPCHandler"
	        
        self._queue = queue.Queue(maxsize = ConstConfig.MAX_GRPC_NUM_MSGS * 32)
        self._meta_dict = meta_dict
        self._translator = translator
        self._tgt_addr = tgt_addr
        self._tgt_port = tgt_port
	
        self._steering = Steering()
        self._steering_on = steering	
        
        if self._steering_on == 1:
            print("steering is on...")

    def __del__(self):
        self._queue.join()
    
    def get_queue(self):
        return self._queue

    async def handle(self, tgt_addr, tgt_port):
        async with PrefetchClient(tgt_addr, tgt_port) as client:
            while True:
                msg_list = self._queue.get()

                if self._steering_on == 1:
                    #print(msg_list)
                    #prev_len = len(msg_list)
                    steered_msg_list = self._steering.process(msg_list)
                    #cur_len = len(steered_msg_list)
                    #print(f"steering: {prev_len} => {cur_len}")
                    msg_list = steered_msg_list

                for info in msg_list:
                    #print(f"info {info.ino} {info.lba}")
                    subsys_id, ns_id = self._meta_dict.get_prefetch_meta(info.dev_id)
                    if subsys_id is None or ns_id is None: # nvme may be umounted
                        continue

                    pba = self._translator.trans_pba(info)
                    if pba is None: # not found in fast path
                        Stats.trans_pba_failed += 1
                        continue
                    
                    # TODO: extent size is determined by exchange metadata
                    # only send extent aligned pba
                    """
                    if pba % ConstConfig.BLOCKS_PER_EXTENT:
                        Stats.trans_pba_extent_filtered += 1
                        continue
                    """

                    Stats.trans_pba_fast += 1
                    Stats.qsize = self._queue.qsize()
                    
                    await client.send_msgs(TransInfo(subsys_id, ns_id, pba))

                self._queue.task_done()

    def run(self):
        asyncio.run(self.handle(self._tgt_addr, self._tgt_port))


# TODO : test program
def main():
    logging.basicConfig() 

if __name__ == '__main__':
    main()
