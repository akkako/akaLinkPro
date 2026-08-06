#include "hid_interface.h"
#include "usb_composite.h"

#if CONFIG_CHERRYDAP_USE_CUSTOM_HID

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_read_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_write_buffer[HID_PACKET_SIZE];


void hid_custom_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    (void)arg;

    switch (event) {
        case USBD_EVENT_CONFIGURED:
            usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);
            break;
        case USBD_EVENT_RESET:
            memset(HID_write_buffer, 0, HID_PACKET_SIZE);
            memset(HID_read_buffer, 0, HID_PACKET_SIZE);
            break;
        default:
            break;
    }
}

void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    (void) ep;
    USB_LOG_DBG("actual in len:%d\r\n", nbytes);
}

void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);// 重新开启读取
}

#endif
