#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include <stdint.h>

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif
#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE __attribute__ ((always_inline)) static inline
#endif
#ifndef __WEAK
#define __WEAK __attribute__ ((weak))
#endif

/************************ DAP 相关配置 ************************/
#define DAP_JTAG_DEV_CNT (8U)             // JTAG 扫描链最大设备数量（1~255）
#define DAP_DEFAULT_PORT (1U)             // 默认接口模式（1 = SWD, 2 = JTAG）
#define DAP_DEFAULT_SWJ_CLOCK (1000000U)  // 默认接口时钟频率（Hz）
#define DAP_PACKET_SIZE (512U)            // DAP 包大小
#define DAP_PACKET_COUNT (4U)             // DAP 包缓冲数量（2-255）
#define TIMESTAMP_CLOCK (0U)              // 时间戳计时频率（Hz）（0 = 不支持）

/************************ 设备信息字符串 ************************/
#define DAP_STR_VENDOR_NAME ("akako")               // 厂商名称字符串
#define DAP_STR_PRODUCT_NAME ("akaLink CMSIS-DAP")  // 设备名称字符串
#define DAP_STR_FIRMWARE_VER ("2.1.1")              // 固件版本字符串

#endif
