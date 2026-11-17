#!/usr/bin/env python3
"""Run as root on a Linux peer on an isolated Ethernet test segment."""
import argparse
import secrets
import socket
import time
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("interface")
parser.add_argument("rei_mac")
args = parser.parse_args()
dst = bytes.fromhex(args.rei_mac.replace(":", ""))
if len(dst) != 6 or dst[0] & 1:
    parser.error("Expected a unicast Rei MAC")
src = bytes.fromhex(Path("/sys/class/net", args.interface, "address").read_text().strip().replace(":", ""))
if src == dst:
    parser.error("Peer and Rei MAC must differ")
nonce = secrets.token_bytes(16)
request = (dst + src + b"\x88\xb5REI-L2-Q" + nonce).ljust(60, b"\0")
expected = (src + dst + b"\x88\xb5REI-L2-A" + nonce).ljust(60, b"\0")
with socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x88B5)) as sock:
    sock.bind((args.interface, 0))
    deadline = time.monotonic() + 10
    for _ in range(5):
        sock.send(request)
        end = min(deadline, time.monotonic() + 2)
        while time.monotonic() < end:
            sock.settimeout(max(0.001, end - time.monotonic()))
            try:
                reply = sock.recv(1600)
            except socket.timeout:
                break
            if reply[:60] == expected:
                print("REI Ethernet RX/TX PASS: matching nonce returned")
                raise SystemExit(0)
raise SystemExit("REI Ethernet RX/TX not confirmed (timeout)")
