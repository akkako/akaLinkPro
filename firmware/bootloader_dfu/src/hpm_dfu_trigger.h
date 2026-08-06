/*
 * HPM DFU Boot Port — trigger, jump, and boot entry API.
 */
#ifndef HPM_DFU_TRIGGER_H
#define HPM_DFU_TRIGGER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check + clear DFU trigger from retention register */
bool hpm_dfu_check_and_clear_trigger(void);

/* Write trigger magic and reset — call from APP */
void hpm_dfu_reboot_to_dfu(void) __attribute__((noreturn));

/* Jump to APP (skips 4-byte DFU signature) — never returns */
void hpm_dfu_jump_to_app(void) __attribute__((noreturn));

/* Check boot conditions: pin, trigger, signature.
 * Jumps to APP if valid, returns only if entering DFU mode. */
void hpm_dfu_check_bootloader_request(void);

/* Millisecond delay */
void hpm_dfu_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif
