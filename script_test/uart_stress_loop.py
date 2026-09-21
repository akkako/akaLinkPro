import os
import sys
import time
import serial

# Continuous UART<->CDC loopback stress for a fixed duration, with error
# accounting. Used together with the SWD benchmark to check whether running
# both at full load drops data.
#
# The read timeout is generous (3 s) and a chunk that arrives completely but
# later than 2x its wire time is counted as "late" rather than an error, so the
# result distinguishes real data loss from flush latency.
#
# Usage: python uart_stress_loop.py <port> <baud> <duration_s> [chunk]

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 9000000
DURATION = float(sys.argv[3]) if len(sys.argv) > 3 else 60.0
CHUNK = int(sys.argv[4]) if len(sys.argv) > 4 else 4096

ser = serial.Serial(PORT, BAUD, timeout=0.2, write_timeout=3)

tx_bytes = 0
rx_bytes = 0
errors = 0
late = 0
max_latency = 0.0
first_err = ""
wire_time = CHUNK * 10.0 / BAUD
deadline = time.time() + DURATION

try:
    while time.time() < deadline:
        data = os.urandom(CHUNK)
        try:
            ser.write(data)
            ser.flush()
        except Exception as e:
            errors += 1
            if not first_err:
                first_err = "write: %s" % e
            try:
                ser.reset_input_buffer()
            except Exception:
                pass
            continue
        tx_bytes += len(data)

        t0 = time.time()
        rx = bytearray()
        while len(rx) < len(data) and (time.time() - t0) < 3.0:
            c = ser.read(len(data) - len(rx))
            if c:
                rx += c
        dt = time.time() - t0
        rx_bytes += len(rx)

        if bytes(rx) != data:
            errors += 1
            if not first_err:
                i = next((k for k in range(min(len(rx), len(data))) if rx[k] != data[k]), min(len(rx), len(data)))
                first_err = "mismatch rx=%d/%d first_diff=%d" % (len(rx), len(data), i)
            ser.reset_input_buffer()
        elif dt > wire_time * 2.0:
            late += 1
            if dt > max_latency:
                max_latency = dt
finally:
    ser.close()

print("STRESS_RESULT baud=%d tx=%d rx=%d errors=%d late=%d max_latency=%.3fs first_err=%s" %
      (BAUD, tx_bytes, rx_bytes, errors, late, max_latency, first_err if first_err else "-"))
sys.exit(1 if errors else 0)
