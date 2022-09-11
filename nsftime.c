#include <stdio.h>
#include <stdlib.h>
#include "nsf.h"
#include "nsfreader_file.h"

#define SAMPLE_RATE 44100
#define OVERSAMPLE_RATE 1
#define NSF_CACHE_SIZE 4096
#define NSF_SAMPLE_LIMIT (100 * SAMPLE_RATE) // 100 seconds


typedef struct loop_detect_s
{
    unsigned long samples;
    uint16_t rom_lo;
    uint16_t rom_hi;
} loop_detect_t;


static void ld_init(loop_detect_t *ld)
{
    ld->samples = 0;
    ld->rom_lo = 0xFFFF;
    ld->rom_hi = 0x0000;
}


static void ld_add_sample(loop_detect_t *ld)
{
    ++(ld->samples);
}


static void ld_apu_read_rom(uint16_t addr, void *param)
{
    loop_detect_t *ld = (loop_detect_t *)param;
    if (addr > ld->rom_hi) ld->rom_hi = addr;
    if (addr < ld->rom_lo) ld->rom_lo = addr;
}


static void ld_apu_write_reg(uint16_t addr, uint8_t val, void *param)
{

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

    ld_init(&ld);

    // Create NSF emulator
    nsf_t* nsf = nsf_create();

    nsf_start_emu(nsf, reader, 1000, SAMPLE_RATE, OVERSAMPLE_RATE);
    nesbus_dump_handlers(nsf->bus);
    nsf_init_song(nsf, song);
    nsf_enable_slience_detect(nsf, 1000);
    nsf_enable_apu_sniffing(nsf, true, ld_apu_read_rom, ld_apu_write_reg, (void *)&ld);

    unsigned long nsamples = 0;
    int16_t sample;
    while (!nsf_silence_detected(nsf) && (nsamples < NSF_SAMPLE_LIMIT))
    {
        nsf_get_samples(nsf, 1, &sample);
        ld_add_sample(&ld);
        ++nsamples;
    }

    printf("Total %lu samples\n", ld.samples);
    printf("APU read rom 0x%04X-0x%04X\n", ld.rom_lo, ld.rom_hi);

    nsf_destroy(nsf);

    nfr_destroy(reader);

    return 0;
}