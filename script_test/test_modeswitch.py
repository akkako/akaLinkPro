import sys
import time
import usb.core
import usb.util
import serial

VID, PID = 0x0D28, 0x0204
# COM port passed as argv[1], defaults to COM75.
PORT = sys.argv[1] if len(sys.argv) > 1 else "COM75"


def com_loop(baud=115200, n=64, timeout=0.8):
    try:
        s = serial.Serial(PORT, baud, timeout=0.1, write_timeout=1)
    except Exception as e:
        return "OPEN_FAIL:%s" % e
    s.reset_input_buffer()
    time.sleep(0.1)
    data = bytes((i * 7 + 3) & 0xFF for i in range(n))
    try:
        s.write(data)
        s.flush()
    except Exception as e:
        s.close()
        return "WRITE_FAIL:%s" % e
    rx = b""
    deadline = time.time() + timeout
    while len(rx) < n and time.time() < deadline:
        c = s.read(n - len(rx))
        if c:
            rx += c
    s.close()
    if rx == data:
        return "LOOPBACK_OK"
    return "LOOPBACK_FAIL got=%d/%d" % (len(rx), n)


def dap_cmd(ep_out, ep_in, payload, timeout=1500):
    ep_out.write(payload, timeout)
    try:
        return bytes(ep_in.read(512, timeout))
    except usb.core.USBTimeoutError:
        return b""


def main():
    dev = usb.core.find(idVendor=VID, idProduct=PID)
    assert dev is not None, "device not found"
    try:
        dev.set_configuration()
    except usb.core.USBError as e:
        print("set_configuration:", e)

    cfg = dev.get_active_configuration()
    intf = usb.util.find_descriptor(cfg, bInterfaceNumber=0)
    ep_out = usb.util.find_descriptor(
        intf, custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_OUT)
    ep_in = usb.util.find_descriptor(
        intf, custom_match=lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == usb.util.ENDPOINT_IN)
    print("ep_out=%s ep_in=%s" % (ep_out, ep_in))

    print("COM (idle/no debug)      :", com_loop())

    # DAP_Connect JTAG (0x02, port=2)
    r = dap_cmd(ep_out, ep_in, bytes([0x02, 0x02]))
    print("DAP_Connect JTAG resp    :", r.hex())
    time.sleep(0.2)
    print("COM (JTAG active)        :", com_loop())

    # DAP_Connect SWD (0x02, port=1)
    r = dap_cmd(ep_out, ep_in, bytes([0x02, 0x01]))
    print("DAP_Connect SWD resp     :", r.hex())
    time.sleep(0.2)
    print("COM (SWD active)         :", com_loop())

    # DAP_Disconnect (0x03)
    r = dap_cmd(ep_out, ep_in, bytes([0x03]))
    print("DAP_Disconnect resp      :", r.hex())
    time.sleep(0.2)
    print("COM (after disconnect)   :", com_loop())


if __name__ == "__main__":
    main()
