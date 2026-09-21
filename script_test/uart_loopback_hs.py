import os
import sys
import time
import threading
import serial

# High-speed UART loopback test.
# HPM5301 UART baud limit = uart_clock / 8. With the current UART2 clock of
# PLL0CLK0(720MHz)/8 = 90MHz -> max = 11.25 Mbps.
PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
TOTAL = int(sys.argv[2]) if len(sys.argv) > 2 else 256 * 1024
BAUDS = [2000000, 3000000, 3750000, 4500000, 5000000, 5625000,
         6000000, 7500000, 9000000, 10000000, 11250000]
WRITE_CHUNK = 8192


def sanity(ser):
    """Quick echo check; detects a baud the target could not apply."""
    pat = bytes((i * 13 + 5) & 0xFF for i in range(64))
    ser.reset_input_buffer()
    time.sleep(0.1)
    try:
        ser.write(pat)
        ser.flush()
    except Exception:
        return False
    rx = b""
    deadline = time.time() + 0.5
    while len(rx) < len(pat) and time.time() < deadline:
        c = ser.read(len(pat) - len(rx))
        if c:
            rx += c
    return rx == pat


def stream_test(ser, total):
    data = os.urandom(total)
    rx = bytearray()
    lock = threading.Lock()
    done = threading.Event()

    def reader():
        while not done.is_set() and len(rx) < total:
            try:
                c = ser.read(4096)
            except Exception:
                break
            if c:
                with lock:
                    rx.extend(c)
        done.set()

    t = threading.Thread(target=reader)
    t.start()
    t0 = time.time()
    sent = 0
    while sent < total:
        n = min(WRITE_CHUNK, total - sent)
        try:
            ser.write(data[sent:sent + n])
            ser.flush()
        except Exception as e:
            done.set()
            t.join(timeout=1)
            return False, "write fail @%d: %s" % (sent, e), 0.0
        sent += n
    # Expected time on the wire is total*10 bits / baud; allow 3x plus margin.
    baud = ser.baudrate if ser.baudrate else 115200
    done.wait(timeout=(total * 10.0 / baud) * 3.0 + 5.0)
    t1 = time.time()
    t.join(timeout=1)
    with lock:
        got = bytes(rx)
    if got != data:
        i = next((k for k in range(min(len(got), len(data))) if got[k] != data[k]), min(len(got), len(data)))
        return False, "mismatch: rx=%d/%d first_diff=%d" % (len(got), total, i), (total / max(t1 - t0, 1e-6))
    return True, "", total / (t1 - t0)


def main():
    print("HS loopback on %s, %d bytes/baud" % (PORT, TOTAL))
    for baud in BAUDS:
        try:
            ser = serial.Serial(PORT, baud, timeout=0.2, write_timeout=5)
        except Exception as e:
            print("  %9d : OPEN FAIL %s" % (baud, e))
            continue
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.1)
        if not sanity(ser):
            print("  %9d : NO ECHO (baud not applied / not supported?)" % baud)
            ser.close()
            continue
        ok, info, bps = stream_test(ser, TOTAL)
        ser.close()
        eff = bps * 10.0 / baud * 100.0  # 10 bits per byte on the wire
        if ok:
            print("  %9d : OK  %8.1f KB/s  (wire eff %.0f%%)" % (baud, bps / 1024.0, eff))
        else:
            print("  %9d : FAIL %s" % (baud, info))


if __name__ == "__main__":
    main()
