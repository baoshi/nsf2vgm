#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nsfrip_record_s
{
    uint32_t wait_samples;
    uint32_t reg_ops;
    unsigned long samples;
} nsfrip_record_t;

typedef struct nsfrip_s
{
    unsigned long total_samples;
    uint16_t rom_lo;
    uint16_t rom_hi;
    unsigned int wait_samples;
    nsfrip_record_t *records;
    unsigned long records_len;
    unsigned long loop_start_idx;
    unsigned long loop_end_idx;
    unsigned long max_records;
} nsfrip_t;


nsfrip_t * nsfrip_create(unsigned long max_records);
void nsfrip_destroy(nsfrip_t *rip);
void nsfrip_add_sample(nsfrip_t *rip);
void nsfrip_finish_rip(nsfrip_t *rip);
void nsfrip_dump(nsfrip_t *rip, unsigned long records);
bool nsfrip_find_loop(nsfrip_t *rip, unsigned long min_length);
void nsfrip_trim_loop(nsfrip_t *rip);
void nsfrip_trim_silence(nsfrip_t *rip, uint32_t samples);

// For use with nsfbus 
void nsfrip_apu_read_rom(uint16_t addr, void *param);
void nsfrip_apu_write_reg(uint16_t addr, uint8_t val, void *param);


// From nsfrip to VGM

#define RIP2VGM_ERR_SUCCESS         0
#define RIP2VGM_ERR_OUTOFMEMORY     -1
#define RIP2VGM_ERR_FILEIO          -2

typedef struct vgm_meta_s
{
    const char *game_name_en;
    const char *track_name_en;
    const char *system_name_en;
    const char *author_name_en;
    const char *release_date;
    const char *creator_name;
    const char *notes;
} vgm_meta_t;

int  nsfrip_export_vgm(nsfrip_t *rip, uint8_t *rom, uint16_t rom_len, vgm_meta_t *info, char const *vgm);


#ifdef __cplusplus
}
#endif
