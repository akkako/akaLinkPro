#include "usb_composite.h"
#include "api_param.h"

#if CONFIG_CHERRYDAP_USE_CUSTOM_HID

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_rx_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t hid_tx_buffer[HID_PACKET_SIZE];

void hid_custom_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    (void)arg;

    switch (event)
    {
    case USBD_EVENT_CONFIGURED:
        usbd_ep_start_read(0, HID_OUT_EP, hid_rx_buffer, HID_PACKET_SIZE);
        break;
    case USBD_EVENT_RESET:
        memset(hid_tx_buffer, 0, HID_PACKET_SIZE);
        memset(hid_rx_buffer, 0, HID_PACKET_SIZE);
        break;
    default:
        break;
    }
}

void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;
}

void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    api_param_proc_hid(hid_rx_buffer, hid_tx_buffer);

    hid_tx_buffer[0] = 0x02; /* IN: report id */

    usbd_ep_start_read(busid, ep, hid_rx_buffer, HID_PACKET_SIZE);
    usbd_ep_start_write(busid, HID_IN_EP, hid_tx_buffer, HID_PACKET_SIZE);
}

#endif
