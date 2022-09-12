#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rip_record_s
{
    uint32_t wait_samples;
    uint32_t reg_ops;
} rip_record_t;

typedef struct nsfrip_s
{
    unsigned long samples;
    uint16_t rom_lo;
    uint16_t rom_hi;
    unsigned int wait_samples;
    rip_record_t *records;
    unsigned long record_idx;
    unsigned long loop_start_idx;
    unsigned long loop_end_idx;
    unsigned long max_records;
} nsfrip_t;


nsfrip_t * nsfrip_create(unsigned long max_records);
void nsfrip_destroy(nsfrip_t *rip);
void nsfrip_add_sample(nsfrip_t *rip);
void nsfrip_dump(nsfrip_t *rip, unsigned long records);
bool nsfrip_find_loop(nsfrip_t *rip, unsigned long min_length);
void nsfrip_trim_loop(nsfrip_t *rip);


// For use with nsfbus 
void nsfrip_apu_read_rom(uint16_t addr, void *param);
void nsfrip_apu_write_reg(uint16_t addr, uint8_t val, void *param);


#ifdef __cplusplus
}
#endif
