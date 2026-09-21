import os
import sys
import time
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
BAUDS = [115200, 460800, 921600, 1000000, 2000000]
CHUNK = 4096
FAIL = 0


def first_diff(a, b):
    n = min(len(a), len(b))
    for i in range(n):
        if a[i] != b[i]:
            return i, a[i], b[i]
    return n, None, None


def test_chunked(ser, total, chunk):
    sent = 0
    while sent < total:
        n = min(chunk, total - sent)
        data = os.urandom(n)
        ser.reset_input_buffer()
        ser.write(data)
        ser.flush()
        rx = bytearray()
        deadline = time.time() + 3.0
        while len(rx) < n and time.time() < deadline:
            c = ser.read(n - len(rx))
            if c:
                rx += c
        if bytes(rx) != data:
            i, a, b = first_diff(data, bytes(rx))
            return False, "chunk@%d len rx=%d/%d first_diff=%d exp=%s got=%s" % (
                sent, len(rx), n, i,
                ("%02X" % a) if a is not None else "-",
                ("%02X" % b) if b is not None else "-")
        sent += n
    return True, ""


def test_byteloop(ser, count):
    for i in range(count):
        d = bytes([i & 0xFF])
        ser.write(d)
        ser.flush()
        rx = ser.read(1)
        if rx != d:
            return False, "byte %d expected %02X got %s" % (i, i & 0xFF, rx.hex() if rx else "none")
    return True, ""


def main():
    global FAIL
    print("Loopback test on %s" % PORT)
    for baud in BAUDS:
        try:
            ser = serial.Serial(PORT, baud, timeout=1, write_timeout=5)
        except Exception as e:
            print("  %7d baud: OPEN FAILED: %s" % (baud, e))
            FAIL += 1
            continue
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.2)

        t0 = time.time()
        ok, info = test_byteloop(ser, 256)
        if not ok:
            print("  %7d baud: BYTE LOOP FAILED: %s" % (baud, info))
            FAIL += 1

        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        t1 = time.time()
        ok2, info2 = test_chunked(ser, 256 * 1024, CHUNK)
        t2 = time.time()
        rate = (256 * 1024) / max(t2 - t1, 1e-6)
        if not ok2:
            print("  %7d baud: STREAM FAILED: %s" % (baud, info2))
            FAIL += 1
        else:
            print("  %7d baud: OK  bytes=%d stream=%.1f KB/s (byte loop %.0f ms)" %
                  (baud, 256 * 1024, rate / 1024.0, (t1 - t0) * 1000))
        ser.close()

    print("RESULT: %s" % ("PASS" if FAIL == 0 else "FAIL (%d)" % FAIL))
    sys.exit(1 if FAIL else 0)


if __name__ == "__main__":
    main()
