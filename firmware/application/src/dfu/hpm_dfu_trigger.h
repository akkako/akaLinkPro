/*
 * HPM DFU Trigger - retention register based DFU entry
 * Allows APP to request bootloader mode across reset via BGPR/PDGO magic.
 */
#ifndef HPM_DFU_TRIGGER_H
#define HPM_DFU_TRIGGER_H

#include <stdbool.h>
#include <stdint.h>
#include "usbd_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Check and clear DFU trigger magic from retention register.
 * Returns true if APP requested DFU mode before last reset. */
bool hpm_dfu_check_and_clear_trigger(void);

/* Write DFU trigger magic and reset — call from APP to enter DFU mode. */
void hpm_dfu_reboot_to_dfu(void) __attribute__((noreturn));

int dfu_runtime_handler(uint8_t busid, struct usb_setup_packet *setup,
                               uint8_t **data, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif
