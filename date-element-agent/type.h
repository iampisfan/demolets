#ifndef _TYPE_H_
#define _TYPE_H_

#include <stdint.h>

#define ETH_ALEN 6
#define COLLECTION_INTERVAL 0

typedef uint8_t							u8;
typedef int8_t							s8;
typedef uint16_t						u16;
typedef uint32_t						u32;
typedef int32_t							s32;
typedef uint64_t						u64;
typedef unsigned char					byte;
typedef enum { FALSE = 0, TRUE = 1 }	Boolean;

#define FLAG_NONE			0x0
#define FLAG_STATIC			0x1 << 0
#define FLAG_UPDATE_YANG	0x1 << 1
#define FLAG_LIST_KEY		0x1 << 2
#define FLAG_LIST_EMPTY		0x1 << 3
#define FLAG_FREE_VALUE		0x1 << 4
#define FLAG_ALLOW_EMPTY_STR 0x1 << 5

#define BIT(x) (1ULL<<(x))

#endif
