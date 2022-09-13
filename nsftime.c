#include <stdio.h>
#include <stdlib.h>
#include "nsf.h"
#include "nsfreader_file.h"
#include "nsfrip.h"
#include "rip2vgm.h"

#define SAMPLE_RATE 44100
#define OVERSAMPLE_RATE 1
#define NSF_CACHE_SIZE 4096
#define NSF_SAMPLE_LIMIT (300 * SAMPLE_RATE) // 100 seconds

#define NSF_MAX_RECORDS  10000000


int main(int argc, char* argv[])
{
    if (argc < 3) return -1;

    char *file = argv[1];
    int song = atoi(argv[2]);

    nsfreader_t *reader = NULL;
    nsfrip_t *rip = NULL;
    nsf_t *nsf = NULL;
    uint8_t *rom = NULL;
    uint16_t rom_len = 0;
    
    do
    {
        reader = nfr_create(argv[1], NSF_CACHE_SIZE);
        if (NULL == reader)
        {
            printf("Failed to create reader\n");
            break;
        }
        rip = nsfrip_create(NSF_MAX_RECORDS);
        if (NULL == rip)
        {
            printf("Failed to create rip\n");
            break;
        }
        nsf = nsf_create();
        if (NULL == nsf)
        {
            printf("Failed to create nsf\n");
            break;
        }
        nsf_start_emu(nsf, reader, 1000, SAMPLE_RATE, OVERSAMPLE_RATE);
        nsf_enable_apu_sniffing(nsf, true, nsfrip_apu_read_rom, nsfrip_apu_write_reg, (void*)rip);
        nsf_enable_slience_detect(nsf, 1000);
        nsf_init_song(nsf, song);
        unsigned long nsamples = 0;
        int16_t sample;
        while (!nsf_silence_detected(nsf) && (nsamples < NSF_SAMPLE_LIMIT))
        {
            nsf_get_samples(nsf, 1, &sample);
            nsfrip_add_sample(rip);
            ++nsamples;
        }
        nsfrip_finish_rip(rip);
        // no more ripping so we can read rom later without being ripped
        nsf_enable_apu_sniffing(nsf, false, NULL, NULL, NULL);
        //nsfrip_dump(rip, 0);
        // if play is finished because of silence detected, no need to find loop
        if (!nsf_silence_detected(nsf))
        {
            if (nsfrip_find_loop(rip, 1000))
            {
                nsfrip_trim_loop(rip);
            }
        }
        // If APU uses rom samples, dump it
        if (rip->rom_hi > rip->rom_lo)
        {
            rom_len = rip->rom_hi - rip->rom_lo + 1;
            rom = malloc(rom_len);
            if (NULL == rom)
            {
                printf("Allocate ROM failed\n");
                break;
            }
            nsf_dump_rom(nsf, rip->rom_lo, rom_len, rom);
        }
        // Convert VGM
        rip2vgm(rip, rom, rom_len, NULL, "test.vgm");
    } while (0);

    if (rom) free(rom);
    if (nsf) nsf_destroy(nsf);
    if (rip) nsfrip_destroy(rip);
    if (reader) nfr_destroy(reader);
    return 0;
}
