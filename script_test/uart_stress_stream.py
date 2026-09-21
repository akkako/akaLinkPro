import os
import sys
import time
import threading
import serial

# Continuous UART<->CDC loopback stress with a reader thread (no per-chunk
# discard). This is the realistic "full load" scenario: the host keeps the
# UART busy and compares the whole echoed stream, so it reports real data loss
# rather than the per-chunk idle-boundary artefacts of uart_stress_loop.py.
#
# Usage: python uart_stress_stream.py <port> <baud> <duration_s> [chunk]

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 9000000
DURATION = float(sys.argv[3]) if len(sys.argv) > 3 else 60.0
CHUNK = int(sys.argv[4]) if len(sys.argv) > 4 else 4096
PATTERN_LEN = 256 * 1024

pattern = os.urandom(PATTERN_LEN)

ser = serial.Serial(PORT, BAUD, timeout=0.2, write_timeout=5)
ser.reset_input_buffer()
ser.reset_output_buffer()

rx = bytearray()
lock = threading.Lock()
done = threading.Event()
sent = [0]


def reader():
    while not done.is_set():
        try:
            c = ser.read(65536)
        except Exception:
            break
        if c:
            with lock:
                rx.extend(c)


th = threading.Thread(target=reader, daemon=True)
th.start()

t0 = time.time()
pos = 0
write_err = None
while time.time() - t0 < DURATION:
    if pos + CHUNK <= PATTERN_LEN:
        data = pattern[pos:pos + CHUNK]
    else:
        data = pattern[pos:] + pattern[:CHUNK - (PATTERN_LEN - pos)]
    try:
        ser.write(data)
        ser.flush()
    except Exception as e:
        write_err = str(e)
        break
    pos = (pos + CHUNK) % PATTERN_LEN
    sent[0] += CHUNK

# Give the echo a generous chance to catch up.
deadline = time.time() + (sent[0] * 10.0 / BAUD) * 2.0 + 10.0
while len(rx) < sent[0] and time.time() < deadline:
    time.sleep(0.05)
time.sleep(0.3)
done.set()
th.join(timeout=1.0)

got = bytes(rx)
exp = (pattern * (sent[0] // PATTERN_LEN + 1))[:sent[0]]
ok = (write_err is None) and (len(got) >= sent[0]) and (got[:sent[0]] == exp)
if ok:
    first = sent[0]
else:
    n = min(len(got), len(exp))
    first = next((i for i in range(n) if got[i] != exp[i]), n)

print("STREAM_RESULT baud=%d sent=%d rx=%d first_diff=%d write_err=%s %s" %
      (BAUD, sent[0], len(got), first, write_err if write_err else "-",
       "OK" if ok else "MISMATCH"))
sys.exit(0 if ok else 1)
