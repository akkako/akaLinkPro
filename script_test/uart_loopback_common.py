import os
import sys
import time
import threading
import serial

# Common-speed UART loopback test with detailed per-baud reporting.
#
# Firmware policy being validated:
#   * requested baud is clamped to UART2_MAX_BAUDRATE (9,000,000)
#   * if it cannot be generated exactly it is rounded to the closest
#     achievable baud = uart_clk / (div * osc), osc in {8..30 even}
UART_CLK = 90000000          # PLL0CLK0(720MHz) / 8
UART_MAX_BAUDRATE = 9000000

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
BAUDS = [9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600,
         1000000, 1500000, 2000000, 2500000, 3000000, 4000000,
         6000000, 8000000, 9000000, 10000000, 11250000]
WRITE_CHUNK = 8192


def expected_baud(req):
    """Mirror of uart2_round_baudrate() in cdc_interface.c."""
    target = min(req, UART_MAX_BAUDRATE)
    best = 0
    best_err = None
    for osc in range(8, 31, 2):
        denom = target * osc
        div = (UART_CLK + denom // 2) // denom
        if div < 1:
            div = 1
        if div > 0xFFFF:
            continue
        actual = UART_CLK // (div * osc)
        if actual == 0 or actual > UART_MAX_BAUDRATE:
            continue
        err = abs(actual - target)
        if best_err is None or err < best_err:
            best_err = err
            best = actual
    return best or target


def test_size(actual):
    # ~0.1 s of wire data, clamped to [1 KiB, 256 KiB]
    n = actual // 10
    return max(1024, min(262144, n))


def sanity(ser):
    pat = bytes((i * 13 + 5) & 0xFF for i in range(64))
    ser.reset_input_buffer()
    time.sleep(0.05)
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


def stream_test(ser, total, actual):
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
    done.wait(timeout=(total * 10.0 / actual) * 3.0 + 5.0)
    t1 = time.time()
    t.join(timeout=1)
    with lock:
        got = bytes(rx)
    if got != data:
        i = next((k for k in range(min(len(got), len(data))) if got[k] != data[k]), min(len(got), len(data)))
        return False, "mismatch rx=%d/%d first_diff=%d" % (len(got), total, i), total / max(t1 - t0, 1e-6)
    return True, "", total / max(t1 - t0, 1e-6)


def main():
    print("Common-speed loopback on %s  (uart_clk=%d, cap=%d)" % (PORT, UART_CLK, UART_MAX_BAUDRATE))
    print("%10s %10s %8s %8s %9s %9s %8s  %s" %
          ("request", "applied", "err", "size", "time(s)", "KB/s", "wire%", "result"))
    fails = 0
    for req in BAUDS:
        act = expected_baud(req)
        size = test_size(act)
        err = (act - req) * 100.0 / req
        try:
            ser = serial.Serial(PORT, req, timeout=0.2, write_timeout=5)
        except Exception as e:
            print("%10d %10d %7.2f%% %8d %9s %9s %7s  OPEN FAIL %s" % (req, act, err, size, "-", "-", "-", e))
            fails += 1
            continue
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.05)
        if not sanity(ser):
            ser.close()
            print("%10d %10d %7.2f%% %8d %9s %9s %7s  NO ECHO" % (req, act, err, size, "-", "-", "-"))
            fails += 1
            continue
        ok, info, bps = stream_test(ser, size, act)
        ser.close()
        wire = bps * 10.0 / act * 100.0
        if ok:
            print("%10d %10d %7.2f%% %8d %9.3f %9.1f %7.1f  OK" %
                  (req, act, err, size, size / max(bps, 1e-9), bps / 1024.0, wire))
        else:
            print("%10d %10d %7.2f%% %8d %9s %9s %7s  FAIL %s" % (req, act, err, size, "-", "-", "-", info))
            fails += 1
    print("RESULT: %s" % ("PASS" if fails == 0 else "FAIL (%d)" % fails))


if __name__ == "__main__":
    main()
