#include <stdio.h>
#include <stdlib.h>
#include "nsf.h"
#include "nsfreader_file.h"

#define SAMPLE_RATE 44100
#define OVERSAMPLE_RATE 1
#define NSF_CACHE_SIZE 4096
#define NSF_SAMPLE_LIMIT (500 * SAMPLE_RATE) // 100 seconds


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

    FILE* fd = fopen("sound.bin", "wb");
    if (!fd)
    {
        printf("Cannot write sound.bin\n");
        nfr_destroy(reader);
        return -1;
    }

    // Create NSF emulator
    nsf_t* nsf = nsf_create();

    nsf_start_emu(nsf, reader, 1000, SAMPLE_RATE, OVERSAMPLE_RATE);
    nesbus_dump_handlers(nsf->bus);
    nsf_init_song(nsf, song);
    nsf_enable_slience_detect(nsf, 1000);
    int nsamples = 0;
    int16_t sample;
    while (!nsf_silence_detected(nsf) && (nsamples < NSF_SAMPLE_LIMIT))
    {
        nsf_get_samples(nsf, 1, &sample);
        fwrite(&sample, sizeof(int16_t), 1, fd);
        ++nsamples;
    }

    nsf_destroy(nsf);

    nfr_destroy(reader);

    fclose(fd);

    return 0;
}