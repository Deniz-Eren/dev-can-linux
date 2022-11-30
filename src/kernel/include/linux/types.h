#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <wchar.h>
#include <stdint.h>

typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;

typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;

typedef __s8 s8;
typedef __s16 s16;
typedef __s32 s32;

typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;

struct sk_buff {
	unsigned char* data;
};

#define __KERNEL__
#define __UAPI_DEF_IF_IFNAMSIZ
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO

#endif /* _LINUX_TYPES_H */
