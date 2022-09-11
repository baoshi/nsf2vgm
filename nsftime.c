#include <stdio.h>
#include <stdlib.h>
#include "nsf.h"
#include "nsfreader_file.h"

#define SAMPLE_RATE 44100
#define OVERSAMPLE_RATE 1
#define NSF_CACHE_SIZE 4096
#define NSF_SAMPLE_LIMIT (100 * SAMPLE_RATE) // 100 seconds

#define NSF_MAX_RECORDS  10000000

typedef struct loop_detect_s
{
    unsigned long samples;
    uint16_t rom_lo;
    uint16_t rom_hi;
    unsigned int wait_samples;
    uint32_t* records;
    unsigned long record_idx;
    unsigned long max_records;
} loop_detect_t;


bool ld_init(loop_detect_t *ld, unsigned long max_records)
{
    ld->samples = 0;
    ld->rom_lo = 0xFFFF;
    ld->rom_hi = 0x0000;
    ld->wait_samples = 0;
    ld->record_idx = 0;
    ld->max_records = max_records;
    ld->records = malloc(max_records * sizeof(uint32_t));
    return (ld->records != NULL);
}


void ld_deinit(loop_detect_t* ld)
{
    if (ld->records)
    {
        free(ld->records);
    }
}


static void ld_add_sample(loop_detect_t *ld)
{
    ++(ld->samples);
    ++(ld->wait_samples);
}


static void ld_apu_read_rom(uint16_t addr, void *param)
{
    loop_detect_t *ld = (loop_detect_t *)param;
    if (addr > ld->rom_hi) ld->rom_hi = addr;
    if (addr < ld->rom_lo) ld->rom_lo = addr;
}


static void ld_apu_write_reg(uint16_t addr, uint8_t val, void *param)
{
    loop_detect_t* ld = (loop_detect_t*)param;
    uint32_t record;
    if (ld->wait_samples > 0)
    {
        if (ld->record_idx < ld->max_records)
        {
            record = 0x61000000 + (ld->wait_samples & 0x00ffffff);
            ld->records[ld->record_idx] = record;
            ++(ld->record_idx);
        }
        ld->wait_samples = 0;
    }
    if (ld->record_idx < ld->max_records)
    {
        uint32_t record;
        record = 0xB4000000 | (addr << 8) | val;
        ld->records[ld->record_idx] = record;
        ++(ld->record_idx);
    }
}


int main(int argc, char* argv[])
{
    if (argc < 3) return -1;

    int song = atol(argv[2]);

    nsfreader_t* reader = nfr_create(argv[1], NSF_CACHE_SIZE);
    if (0 == reader)
    {
        printf("Failed to create reader\n");
        return -1;
    }

    loop_detect_t ld;

    ld_init(&ld, NSF_MAX_RECORDS);

    // Create NSF emulator
    nsf_t* nsf = nsf_create();

    nsf_start_emu(nsf, reader, 1000, SAMPLE_RATE, OVERSAMPLE_RATE);
    nsf_enable_apu_sniffing(nsf, true, ld_apu_read_rom, ld_apu_write_reg, (void*)&ld);
    nsf_enable_slience_detect(nsf, 1000);
    nesbus_dump_handlers(nsf->bus);

    nsf_init_song(nsf, song);
  
    unsigned long nsamples = 0;
    int16_t sample;
    while (!nsf_silence_detected(nsf) && (nsamples < NSF_SAMPLE_LIMIT))
    {
        nsf_get_samples(nsf, 1, &sample);
        ld_add_sample(&ld);
        ++nsamples;
    }
    
    printf("Total %lu samples\n", ld.samples);
    printf("Recorded %lu records\n", ld.record_idx + 1);
    printf("APU read rom 0x%04X-0x%04X\n", ld.rom_lo, ld.rom_hi);

    ld_deinit(&ld);

    nsf_destroy(nsf);

    nfr_destroy(reader);

    return 0;
}
