#ifndef __API_PARAM_H__
#define __API_PARAM_H__

#include <stdint.h>

typedef struct _appiparam_t {
    uint32_t magic_number;
    uint8_t output_mode;     // 0 - SWD+VCOM, 1 - SWD+JTAG
    uint8_t usb5v_out_mode;  // 0 - Disable, 1 - Enable
    uint8_t clock_accel_mode;// 0 - Disable, 1 - Enable
    uint8_t led1_mode;       // LED1 (PB11) display mode, 1-9, see led_state.h
    uint8_t led2_mode;       // LED2 (PB12) display mode, 1-9, see led_state.h
    uint16_t vref_mv;        // External reference threshold for mode 5, 1800-5000 mV
} api_param_t;

extern api_param_t g_param;

/* Factory defaults (single source of truth, also used by the EasyFlash port). */
extern const api_param_t g_param_default;

#ifdef __cplusplus
extern "C" {
#endif

extern void api_param_load (void);
extern void api_param_save (void);

/* Request a deferred save from interrupt context; api_param_poll() (main loop)
 * performs the actual flash write outside of the USB ISR. */
extern void api_param_request_save (void);
extern void api_param_poll (void);

/* Apply the RAM configuration to the hardware (5V output, ...). */
extern void api_param_apply (void);

extern void api_param_proc_hid (uint8_t *req_hid, uint8_t *res_hid);

#ifdef __cplusplus
}
#endif

#endif
