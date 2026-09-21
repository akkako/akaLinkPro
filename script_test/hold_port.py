import sys
import time
import serial

# Open the CDC COM port (default COM75) and keep it for a few seconds so the
# target can be inspected with J-Link/GDB while the UART bridge is configured.
PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"
SECONDS = float(sys.argv[2]) if len(sys.argv) > 2 else 8.0

p = serial.Serial(PORT, 115200, timeout=0.5, write_timeout=1)
p.reset_input_buffer()
try:
    p.write(b"\x55")
    p.flush()
except Exception as e:
    print("write err", e)
print("holding %s for %.1fs" % (PORT, SECONDS))
time.sleep(SECONDS)
p.close()
