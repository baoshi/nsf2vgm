#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "nsfrip.h"


#define RECORD_WRITE_REG    0xb4000000


nsfrip_t * nsfrip_create(unsigned long max_records)
{
    nsfrip_t *rip = malloc(sizeof(nsfrip_t));
    if (rip)
    {
        memset(rip, 0, sizeof(nsfrip_t));
        rip->records = malloc(max_records * sizeof(nsfrip_record_t));
        if (!rip->records)
        {
            free(rip);
            rip = NULL;
        }
        else
        {
            rip->total_samples = 0;
            rip->rom_lo = 0xffff;
            rip->rom_hi = 0x0000;
            rip->wait_samples = 0;
            rip->records_len = 0;
            rip->max_records = max_records;
        }
    }
    return rip;
}


void nsfrip_destroy(nsfrip_t *rip)
{
    if (rip)
    {
        if (rip->records)
        {
            free(rip->records);
        }
        free(rip);
    }
}


void nsfrip_add_sample(nsfrip_t *rip)
{
    ++(rip->wait_samples);
}


void nsfrip_finish_rip(nsfrip_t *rip)
{
    // add those samples in waiting
    while ((rip->wait_samples > 65535) && (rip->records_len < rip->max_records))
    {
        rip->total_samples += 65535;
        rip->wait_samples -= 65535;
        rip->records[rip->records_len].wait_samples = 65535;
        rip->records[rip->records_len].reg_ops = 0;
        ++(rip->records_len);
    }
    if (rip->records_len < rip->max_records)
    {
        
        rip->total_samples += rip->wait_samples;
        rip->records[rip->records_len].wait_samples = rip->wait_samples;
        rip->wait_samples = 0;
        rip->records[rip->records_len].reg_ops = 0;
        ++(rip->records_len);
    }
    --(rip->records_len);   // extra 1 added above, take it out
}


void nsfrip_dump(nsfrip_t *rip, unsigned long records)
{
    unsigned long samples = 0;
    if (records == 0) records = rip->records_len;
    for (unsigned long i = 0; i < records; ++i)
    {
        uint32_t record = rip->records[i].reg_ops;
        int16_t addr = (record >> 8) & 0xffff;
        uint8_t val = record & 0xff;
        printf("%06u: Write $%04x : %02x\n", i, addr, val);
    }
}


void nsfrip_apu_read_rom(uint16_t addr, void *param)
{
    // Just keep track the range of ROM APU is reading so we can dump the rom later
    nsfrip_t *rip = (nsfrip_t *)param;
    if (addr > rip->rom_hi) rip->rom_hi = addr;
    if (addr < rip->rom_lo) rip->rom_lo = addr;
}


void nsfrip_apu_write_reg(uint16_t addr, uint8_t val, void *param)
{
    nsfrip_t *rip = (nsfrip_t *)param;
    uint32_t record;
    if (rip->records_len < rip->max_records)
    {
        while (rip->wait_samples > 65535)
        {
            rip->total_samples += 65535;
            rip->wait_samples -= 65535;
            rip->records[rip->records_len].time_stamp = rip->total_samples;
            rip->records[rip->records_len].wait_samples = 65535;
            rip->records[rip->records_len].reg_ops = 0;
            ++(rip->records_len);    
        }
        rip->total_samples += rip->wait_samples;
        rip->records[rip->records_len].wait_samples = rip->wait_samples;
        rip->wait_samples = 0;
        rip->records[rip->records_len].time_stamp = rip->total_samples;
        rip->records[rip->records_len].reg_ops = RECORD_WRITE_REG | (addr << 8) | val;
        ++(rip->records_len);
    }
}


static inline bool compare_records(const nsfrip_record_t *a, const nsfrip_record_t *b)
{
    return (a->reg_ops == b->reg_ops);
}


/*
    Check if:
        records[loop_start                  ] == records[loop_start + loop_length                  ]
        records[loop_start + 1              ] == records[loop_start + loop_length + 1              ]
        records[loop_start + 2              ] == records[loop_start + loop_length + 2              ]
        ...
        records[loop_start + loop_length - 1] == records[loop_start + loop_length + loop_length - 1]
 */
static inline bool is_loop(nsfrip_record_t *records, unsigned long length, unsigned long loop_start, unsigned long loop_length, bool allow_truncate)
{
    unsigned long p1 = loop_start;
    unsigned long p2 = loop_start + loop_length;
    for (p1 = loop_start; p1 < loop_start + loop_length; ++p1)
    {
        p2 = p1 + loop_length;
        if (p2 < length)
        {
            if (!compare_records(&records[p1], &records[p2])) 
                return false;
        }
        else
        {
            if (allow_truncate) break;
        }
    }
    return true;
}


bool nsfrip_find_loop(nsfrip_t *rip, unsigned long min_length)
{
    nsfrip_record_t *records = rip->records;
    unsigned long length = rip->records_len;
    unsigned long start, max_length, loop_length;
    for (start = 0; start < length / 2; ++start)
    {
        max_length = (length - start) / 2;
        for (loop_length = max_length; loop_length > min_length; --loop_length)
        {
            if (is_loop(records, length, start, loop_length, false))
            {
                // it is possible that the loop ls like ABC ABC DEFG DEFG
                // When we found a loop, test if it loops all the way to the end.
                bool isrealloop = true;
                if (start + loop_length + loop_length < length)
                {
                    unsigned long temp_start = start + loop_length;
                    while (temp_start + loop_length < length)
                    {
                        if (!is_loop(records, length, temp_start, loop_length, true))
                        {
                            isrealloop = false;
                            break;
                        }
                        temp_start = temp_start + loop_length;
                    }
                    if (isrealloop)
                    {
                        rip->loop_start_idx = start;
                        rip->loop_end_idx = start + loop_length - 1;
                        return true;
                    }
                    else
                    {
                        start = temp_start; // Skip ABC
                    }
                }
                else
                {
                    rip->loop_start_idx = start;
                    rip->loop_end_idx = start + loop_length - 1;
                    return true;
                }
            }
        }
    }
    return false;
}


void nsfrip_trim_loop(nsfrip_t *rip)
{
    if (rip->loop_end_idx != 0)
        rip->records_len = rip->loop_end_idx;
    unsigned long samples = 0;
    for (unsigned int i = 0; i < rip->records_len; ++i)
    {
        uint32_t wait = rip->records[i].wait_samples;
        samples += wait;
    }
    rip->total_samples = samples;
}
