#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "platform.h"
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
            rip->reg4012 = 0;
            rip->reg4012_valid = false;
            rip->reg4013 = 0;
            rip->reg4013_valid = false;
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
        NSF_PRINTF("%06lu: Write $%04x : %02x\n", i, addr, val);
    }
}


void nsfrip_apu_write_reg(uint16_t addr, uint8_t val, void *param)
{
    nsfrip_t *rip = (nsfrip_t *)param;
    if (rip->records_len < rip->max_records)
    {
        while (rip->wait_samples > 65535)
        {
            rip->total_samples += 65535;
            rip->wait_samples -= 65535;
            rip->records[rip->records_len].samples = rip->total_samples;
            rip->records[rip->records_len].wait_samples = 65535;
            rip->records[rip->records_len].reg_ops = 0;
            ++(rip->records_len);
            if (rip->records_len < rip->max_records)
                break;
        }
        if (rip->records_len < rip->max_records)
        {
            rip->total_samples += rip->wait_samples;
            rip->records[rip->records_len].samples = rip->total_samples;
            rip->records[rip->records_len].wait_samples = rip->wait_samples;
            rip->wait_samples = 0;
            rip->records[rip->records_len].reg_ops = RECORD_WRITE_REG | (addr << 8) | val;
            ++(rip->records_len);
            // Find DMC sample address based on write to $4012 and $4013
            if (addr == 0x4012)
            {
                rip->reg4012 = val;
                rip->reg4012_valid = true;
            }
            if (addr == 0x4013)
            {
                rip->reg4013 = val;
                rip->reg4013_valid = true;
            }
            if (rip->reg4012_valid && rip->reg4013_valid)
            {
                uint16_t dmc_lo = 0xc000 + ((uint16_t)(rip->reg4012) << 6); // Sample address = %11AAAAAA.AA000000 = $C000 + (A * 64)
                uint16_t dmc_hi = dmc_lo + ((uint16_t)(rip->reg4013) << 4); // Sample length = %LLLL.LLLL0001 = (L * 16) + 1 bytes
                if (dmc_hi > rip->rom_hi) rip->rom_hi = dmc_hi;
                if (dmc_lo < rip->rom_lo) rip->rom_lo = dmc_lo;
                rip->reg4012_valid = false;
                rip->reg4013_valid = false;
            }
        }
    }
}


static inline bool compare_records(const nsfrip_record_t *a, const nsfrip_record_t *b)
{
    return (a->reg_ops == b->reg_ops);
}


/*
    Check if:
        records[loop_start                  ] == records[loop_start +     loop_length                  ]
        ...
        records[loop_start                  ] == records[loop_start + k * loop_length                  ] 
        
        records[loop_start + 1              ] == records[loop_start +     loop_length + 1              ]
        ...
        records[loop_start + 1              ] == records[loop_start + k * loop_length + 1              ]
        ...
        ...
        records[loop_start + loop_length - 1] == records[loop_start + k * loop_length + loop_length - 1]
        

 */
static inline bool is_loop(nsfrip_record_t *records, unsigned long length, unsigned long loop_start, unsigned long loop_length)
{
    unsigned long k = ((length - loop_start + 1) / loop_length - 1); // loop_start + (k + 1) * loop_length - 1 < length
    const nsfrip_record_t *a, *b;
    for (unsigned long i = 1; i <= k; ++i)
    {
        for (unsigned long j = 0; j < loop_length; ++j)
        {
            a = &records[loop_start + j];
            b = &records[loop_start + i * loop_length + j];
            if (!compare_records(a, b))
                return false;
        }
    }
    return true;
}


bool nsfrip_find_loop(nsfrip_t *rip, unsigned long min_length)
{
    nsfrip_record_t *records = rip->records;
    // The last ripped record is a pure wait record added in nsfrip_finish_rip. It does not contain register operation.
    // Loop finding shall exclude last record.
    unsigned long length = rip->records_len - 1;

    unsigned long start, max_length, loop_length;

    for (start = 0; start < length / 2; ++start)
    {
        max_length = (length - start) / 2;
        for (loop_length = min_length; loop_length < max_length; ++loop_length)
        {
            if (is_loop(records, length, start, loop_length))
            {
                rip->loop_start_idx = start;
                rip->loop_end_idx = start + loop_length - 1;
                return true;
            }
        }
    }
    return false;
}


// Trim records after loop end
void nsfrip_trim_loop(nsfrip_t *rip)
{
    if (rip->loop_end_idx != 0)
    {
        rip->records_len = rip->loop_end_idx + 1;
        rip->total_samples = rip->records[rip->loop_end_idx].samples;
    }
}


// Trip samples from end of rip structure
void nsfrip_trim_silence(nsfrip_t *rip, uint32_t samples)
{
    unsigned long index = rip->records_len - 1;
    uint32_t total_waits = 0;

    while (total_waits < samples)
    {
        total_waits += rip->records[index].wait_samples;
        --index;
    }
    ++index;
    if (total_waits > samples)
    {
        unsigned long adjust = rip->records[index].wait_samples - (total_waits - samples);
        rip->records[index].wait_samples -= adjust;
        rip->records[index].samples -= adjust;
    }
    rip->records_len = index + 1;
    rip->total_samples -= samples;
}
