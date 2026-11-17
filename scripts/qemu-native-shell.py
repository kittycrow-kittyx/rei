#!/usr/bin/env python3
"""Exercise the actual native guest shell over QEMU UART."""
import os
from pathlib import Path
import secrets
import socket
import subprocess
import sys
import tempfile
import time

image = Path(sys.argv[1]).resolve(strict=True)
deadline = time.monotonic() + 30
transcript = bytearray()
cursor = 0
with tempfile.TemporaryDirectory(prefix="rei-shell-", dir="/tmp") as directory:
    endpoint = os.path.join(directory, "uart")
    proc = subprocess.Popen(
        ["nice", "-n", "10", "qemu-system-aarch64", "-M", "raspi3b",
         "-accel", "tcg,thread=single", "-display", "none", "-monitor", "none",
         "-serial", f"unix:{endpoint},server=on,wait=on", "-kernel", str(image)],
        stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as uart:
            while True:
                if proc.poll() is not None:
                    raise RuntimeError("QEMU exited before UART connection")
                try:
                    uart.connect(endpoint)
                    break
                except (FileNotFoundError, ConnectionRefusedError):
                    if time.monotonic() >= deadline:
                        raise TimeoutError("QEMU UART did not start")
                    time.sleep(0.05)

            def expect(marker):
                global cursor
                while True:
                    found = transcript.find(marker, cursor)
                    if found >= 0:
                        cursor = found + len(marker)
                        return
                    remaining = deadline - time.monotonic()
                    if remaining <= 0 or len(transcript) > 65536:
                        raise RuntimeError(f"Missing shell marker: {marker!r}")
                    uart.settimeout(remaining)
                    chunk = uart.recv(4096)
                    if not chunk:
                        raise RuntimeError("QEMU UART closed")
                    transcript.extend(chunk)

            expect(b"REI: MIKUOS NATIVE SHELL LOADED")
            expect(b"root@mikuos:~# ")
            nonce = secrets.token_hex(8).encode("ascii")
            uart.sendall(b"echo " + nonce + b"\r")
            # Require an exact output line, not the echoed input command.
            expect(b"\r\n" + nonce + b"\r\n")
            expect(b"root@mikuos:~# ")
            uart.sendall(b"id\r")
            expect(b"\r\nuid=0 gid=0 pid=1\r\n")
            expect(b"root@mikuos:~# ")
            uart.sendall(b"exit\r")
            expect(b"REI: MIKUOS SHELL EXIT")
            print("PASS: native stdin, guest echo, kernel credentials and shell exit")
    finally:
        sys.stdout.buffer.write(transcript)
        sys.stdout.flush()
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
