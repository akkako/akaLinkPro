import os, sys, time, threading, serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
TOTAL = int(sys.argv[3]) if len(sys.argv) > 3 else 32768

DATA = os.urandom(TOTAL)

ser = serial.Serial(PORT, BAUD, timeout=0.2, write_timeout=5)
ser.reset_input_buffer()
ser.reset_output_buffer()
time.sleep(0.2)

rx = bytearray()
lock = threading.Lock()


def reader():
    while len(rx) < TOTAL:
        c = ser.read(4096)
        if c:
            with lock:
                rx.extend(c)


t = threading.Thread(target=reader)
t.start()
ser.write(DATA)
ser.flush()
t.join(timeout=TOTAL / (BAUD / 10.0) * 4 + 5)
ser.close()

print("sent=%d received=%d" % (TOTAL, len(rx)))
n = min(len(rx), TOTAL)
first = next((i for i in range(n) if rx[i] != DATA[i]), n)
print("first_mismatch=%d" % first)
if first < n:
    # print context around mismatch
    lo = max(0, first - 8)
    print("sent around:", DATA[lo:first + 16].hex())
    print("recv around:", bytes(rx[lo:first + 16]).hex())
    # is the rest a shifted copy? find offset after first where recv[i] == DATA[j]
    seg = DATA[first:first + 64]
    for shift in range(-64, 65):
        if bytes(rx[first + shift:first + shift + 64]) == seg:
            print("recv matches sent shifted by %d at this point" % shift)
            break
    # count zero runs
    zr = 0
    maxzr = 0
    for b in rx[first:first + 512]:
        if b == 0:
            zr += 1
            maxzr = max(maxzr, zr)
        else:
            zr = 0
    print("max zero run in next 512 bytes:", maxzr)
