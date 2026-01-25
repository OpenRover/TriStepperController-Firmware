# ==============================================================================
# Author: Yuxuan Zhang (dev@z-yx.cc)
# License: TBD (UNLICENSED)
# ==============================================================================

from time import sleep, time as now
from sys import stderr
from typing import Callable

from .protocol import Method, Prop, PacketChain
from .util import bytes_repr


class Expect:
    def __init__(
        self,
        packets: PacketChain,
        method: Method,
        prop: Prop,
        payload: Callable[[bytes], bool] | None = None,
    ):
        self.packets = packets
        self.method = method
        self.prop = prop
        self.payload = payload

    last_packet: PacketChain | None = None

    @property
    def is_next_packet_valid(self):
        p = self.packets.next
        if not isinstance(p, PacketChain):
            return False
        if p.method != self.method:
            return False
        if p.prop != self.prop:
            return False
        if self.payload is not None and not self.payload(p.payload):
            return False
        return True

    def __call__(self):
        while True:
            if self.is_next_packet_valid:
                return True
            elif self.packets.next is PacketChain.FLAG_TERM:
                raise RuntimeError(
                    "Packet chain terminated before expected packet was received."
                )
            if self.packets.next is not None:
                self.packets = self.packets.next
            return False

    @staticmethod
    def wait(
        *expects: "Expect | Fulfilled",
        timeout: float | None = None,
        tick: Callable | None = None
    ):
        """Wait until all expected packets are received."""
        if timeout is not None:
            ddl = now() + timeout
        else:
            ddl = None
        while True:
            if ddl is not None and now() > ddl:
                return False
            expects = tuple(e for e in expects if not e())
            if len(expects) == 0:
                return True
            if tick is not None:
                tick()
            else:
                sleep(1e-3)

    class Fulfilled:
        def __call__(self):
            return True
