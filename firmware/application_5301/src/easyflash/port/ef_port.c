/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for the HPM5301 (akaLink Pro).
 */

#include <easyflash.h>
#include <stdarg.h>
#include "board.h"
#include "hpm_common.h"
#include "hpm_debug_console.h"
#include "drv_flash.h"
#include "api_param.h"

/* ENV key that holds the whole api_param_t blob. */
#define API_PARAM_ENV_KEY "cfg"

/* Factory defaults, owned by the application layer (single source of truth). */
static const ef_env default_env_set[] = {
    {API_PARAM_ENV_KEY, (void *)&g_param_default, sizeof(api_param_t)},
};

/* Saved interrupt level for the ENV critical section. */
static uint32_t s_irq_level;

/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env const **default_env, size_t *default_env_size)
{
    *default_env = default_env_set;
    *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);
    return EF_NO_ERR;
}

/**
 * Read data from flash.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size)
{
    return (drv_flash_read(addr, buf, (uint32_t)size) == status_success) ? EF_NO_ERR : EF_READ_ERR;
}

/**
 * Erase data on flash.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size)
{
    if ((addr % EF_ERASE_MIN_SIZE) != 0U)
    {
        return EF_ERASE_ERR;
    }
    return (drv_flash_erase(addr, (uint32_t)size) == status_success) ? EF_NO_ERR : EF_ERASE_ERR;
}

/**
 * Write data to flash.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size)
{
    return (drv_flash_write(addr, buf, (uint32_t)size) == status_success) ? EF_NO_ERR : EF_WRITE_ERR;
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void)
{
    s_irq_level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void)
{
    restore_global_irq(s_irq_level);
}

/**
 * This function is print flash debug info.
 */
void ef_log_debug(const char *file, const long line, const char *format, ...)
{
#ifdef PRINT_DEBUG
    va_list args;

    (void)file;
    (void)line;
    va_start(args, format);
    printf("[ef debug] ");
    vprintf(format, args);
    va_end(args);
#else
    (void)file;
    (void)line;
    (void)format;
#endif
}

/**
 * This function is print flash routine info.
 */
void ef_log_info(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    printf("[ef info] ");
    vprintf(format, args);
    va_end(args);
}

/**
 * This function is print flash non-package info.
 */
void ef_print(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
