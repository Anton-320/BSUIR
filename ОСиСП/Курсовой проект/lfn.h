#pragma once
#include <stdint.h>
#include "general_fun.h"
#include "structs.h"

typedef struct {
    uint8_t id;			    // sequence number for slot
    uint8_t name0_4[10];	// first 5 characters in name
    uint8_t attr;		    // attribute byte
    uint8_t reserved;		// always 0
    uint8_t alias_checksum;	// checksum for 8.3 alias
    uint8_t name5_10[12];	// 6 more characters in name
    uint16_t start;		    // starting cluster number, 0 in long slots
    uint8_t name11_12[4];	// last 2 characters in name
} __attribute__ ((packed)) LFN_ENT;

void get_lfn_file(DirEntry* entries);