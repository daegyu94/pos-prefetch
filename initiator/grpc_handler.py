"""The Python AsyncIO implementation of the GRPC helloworld.Greeter client."""

import logging
import asyncio
import grpc
import prefetch_pb2
import prefetch_pb2_grpc
import threading
import time

from stats import *
from my_types import TransInfo

# TODO: process buffered msg with timeout
class PrefetchClient:
    def __init__(self, host, port):
        self._channel = grpc.aio.insecure_channel("{}:{}" .format(host, port))
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
        await self.__send_msgs()
        #if self._num_msgs == MAX_GRPC_NUM_MSGS:
        #    print("send_msgs")
        #    await self.__send_msgs()

    async def __aenter__(self):
        return self
        
    async def __aexit__(self, exc_type, exc_val, exc_tb):
        await self._channel.close()


class gRPCHandler():
    def __init__(self, tgt_addr, tgt_port):
        self.name = "gRPCHandler"
        self._tgt_addr = tgt_addr
        self._tgt_port = tgt_port
	

    async def handle(self, trans_info):
        async with PrefetchClient(self._tgt_addr, self._tgt_port) as client:
            await client.send_msgs(trans_info)
