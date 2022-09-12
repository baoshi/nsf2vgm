#pragma once

#include "nsfrip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vgm_meta_s
{

} vgm_meta_t;


int rip2vgm(nsfrip_t *rip, uint8_t *rom, uint16_t rom_len, vgm_meta_t *info, char const *vgm);


#ifdef __cplusplus
}
#endif