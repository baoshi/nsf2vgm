#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "rip2vgm.h"

#ifdef _MSC_VER
# define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
# else
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
# endif


PACK(struct vgm_header_s
{
    uint32_t ident;             // 0x00: identification "Vgm " 0x206d6756
    uint32_t eof_offset;        // 0x04: relative offset to end of file (should be file_length - 4)
    uint32_t version;           // 0x08: BCD version. Ver 1.7.1 is 0x00000171
    uint32_t sn76489_clk;
    uint32_t ym2413_clk;
    uint32_t gd3_offset;        // 0x14: relative offset to GD3 tag (0 if no GD3 tag
    uint32_t total_samples;     // 0x18: total samples (sum of wait values) in the file
    uint32_t loop_offset;       // 0x1c: relative offset to loop point, or 0 if no loop
    uint32_t loop_samples;      // 0x20: number of samples in one loop. or 0 if no loop
    uint32_t rate;              // 0x24: rate of recording. typeical 50 for PAL and 60 for NTSC
    uint16_t sn76489_feedback;
    uint16_t sn76489_shiftreg;
    uint32_t ym2612_clk;
    uint32_t ym2151_clk;
    uint32_t data_offset;       // 0x34: relative offset to VGM data stream. for version 1.5 and below, its value is 0x0c and data starts at 0x40
    uint32_t sega_pcm_clk;
    uint32_t sega_pcm_if_reg;
    uint32_t rf5c68_clk;
    uint32_t ym2203_clk;
    uint32_t ym2608_clk;
    uint32_t ym2610b_clk;
    uint32_t ym3812_clk;
    uint32_t ym3526_clk;
    uint32_t y8950_clk;
    uint32_t ymf262_clk;
    uint32_t ymf278b_clk;
    uint32_t ymf271_clk;
    uint32_t ymz280b_clk;
    uint32_t rf5c164_clk;
    uint32_t pwm_clock;
    uint32_t ay8910_clk;
    uint8_t  ay8910_type;
    uint8_t  ay8910_flags;
    uint8_t  ym2203_ay8910_flags;
    uint8_t  ym2608_ay8910_flags;
    uint8_t  volume_modifier;   // 0x7c: range [-63 .. 192], volume = 2 ^ (volume_modifier / 0x20)
    uint8_t  reserved1;
    uint8_t  loop_base; 
    uint8_t  loop_modifier;
    uint32_t gb_dmg_clk;
    uint32_t nes_apu_clk;       // 0x84: NES APU Clock (typical 1789772). Bit 31 indicates FDS addition
    uint32_t multi_pcm_clk;
    uint32_t upd7759_clk;
    uint32_t okim6258_clk;
    uint8_t  okim6258_flags;
    uint8_t  k054539_flags;
    uint8_t  c140_flags;
    uint8_t  reserved2;
    uint32_t oki6295_clk;
    uint32_t k051649_clk;
    uint32_t k054539_clk;
    uint32_t huc6280_clk;
    uint32_t c140_clk;
    uint32_t k053260_clk;
    uint32_t pokey_clk;
    uint32_t qsound_clock;
    uint32_t scsp_clk;
    uint32_t extra_header_offset;   // 0xBC: relative offset to the extra header or 0 if no extra header is present
    uint32_t wswan_clk;
    uint32_t vsu_clk;
    uint32_t saa1090_clk;
    uint32_t es5503_clk;
    uint32_t es5506_clk;
    uint8_t  es5503_chns;
    uint8_t  es5506_chns;
    uint8_t  c352_clk_div;
    uint8_t  reserved3;
    uint32_t x1010_clk;
    uint32_t c352_clk;
    uint32_t ga20_clk;
    uint8_t  reserved4[7 * 4];
});
typedef struct vgm_header_s vgm_header_t;



#define RIP2VGM_ERR_SUCCESS         0
#define RIP2VGM_ERR_OUTOFMEMORY     -1
#define RIP2VGM_ERR_FILEIO          -2

int rip2vgm(nsfrip_t *rip, uint8_t *rom, uint16_t rom_len, vgm_meta_t *info, char const *vgm)
{
    int r = RIP2VGM_ERR_SUCCESS;
    uint8_t *stream = NULL;
    FILE *fd = NULL;
    do
    {
        unsigned int loop_pos_rel = 0;
        unsigned int loop_samples = 0; 
        unsigned long total_samples = 0;
        unsigned long stream_len, stream_idx;
        stream_len = rom_len + 9;   // NES APU RAM 0x67 0x66 0xc2 ss ss ss ss (ll ll rom)
        // each record in rip is 4 bytes, maximum expand to 2 VGM commands, records_len * 8 should be enough
        stream_len += rip->records_len * 8;
        stream = malloc(stream_len);
        if (NULL == stream)
        {
            r = RIP2VGM_ERR_OUTOFMEMORY;
            break;
        }
        stream_idx = 0;
        // save rom data
        if (rom_len > 0)
        {
            stream[stream_idx] = 0x67; ++stream_idx;
            stream[stream_idx] = 0x66; ++stream_idx;
            stream[stream_idx] = 0xc2; ++stream_idx;
            stream[stream_idx] = (rom_len + 2) & 0xff; ++stream_idx;
            stream[stream_idx] = ((rom_len + 2) >> 8) & 0xff; ++stream_idx;
            stream[stream_idx] = ((rom_len + 2) >> 16) & 0xff; ++stream_idx;
            stream[stream_idx] = ((rom_len + 2) >> 24) & 0xff; ++stream_idx;
            stream[stream_idx] = rip->rom_lo & 0xff; ++stream_idx;
            stream[stream_idx] = (rip->rom_lo >> 8) & 0xff; ++stream_idx;
            memcpy(stream + stream_idx, rom, rom_len); stream_idx += rom_len;
        }
        // save ripped data to stream
        for (unsigned long i = 0; i < rip->records_len; ++i)
        {
            uint32_t wait = rip->records[i].wait_samples;
            uint32_t reg = rip->records[i].reg_ops;
            if (wait > 0)
            {
                total_samples += wait;
                loop_samples += wait;
                if (735 == wait)
                {
                    stream[stream_idx] = 0x62;  // 0x62 - wait 735 samples
                    ++stream_idx;
                }
                else if (882 == wait)
                {
                    stream[stream_idx] = 0x63;  // 0x63 - wait 882 samples
                    ++stream_idx;
                }
                else if ((wait >= 1) && (wait <= 16))
                {
                    stream[stream_idx] = 0x70 + wait - 1;   // 0x7n - wait n+1 samples
                    ++stream_idx;
                }
                else
                {
                    stream[stream_idx] = 0x61;              // 0x61 nn nn - wait nnnn samples
                    ++stream_idx;
                    stream[stream_idx] = wait & 0xff;
                    ++stream_idx;
                    stream[stream_idx] = (wait >> 8) & 0xff;
                    ++stream_idx;
                }
            }
            if (i == rip->loop_start_idx)
            {
                loop_pos_rel = stream_idx;
                loop_samples = 0;
            }
            if (reg > 0)
            {
                stream[stream_idx] = 0xb4;  // b4 aa dd
                stream[stream_idx + 1] = (reg & 0x0000ff00) >> 8;
                stream[stream_idx + 2] = reg & 0xff;
                stream_idx += 3;
            }
        }
        stream[stream_idx] = 0x66;   // eof of sound data
        ++stream_idx;    
        // vgm header
        vgm_header_t header;
        memset(&header, 0, sizeof(vgm_header_t));
        header.ident = 0x206d6756;
        header.version = 0x00000171;
        header.eof_offset = sizeof(vgm_header_t) + stream_idx - 4;    // change with GD3 is added
        header.gd3_offset = 0;
        header.data_offset = sizeof(vgm_header_t) - 0x34;
        header.total_samples = total_samples;
        if (loop_pos_rel > 0)
        {
            header.loop_offset = sizeof(vgm_header_t) + loop_pos_rel - 0x1c;
            header.loop_samples = loop_samples; 
        }
        else
        {
            header.loop_offset = 0;
            header.loop_samples = 0; 
        }
        header.nes_apu_clk = 1789773;
        fd = fopen(vgm, "wb");
        if (NULL == fd)
        {
            r = RIP2VGM_ERR_FILEIO;
            break;
        }
        fwrite(&header, sizeof(vgm_header_t), 1, fd);
        fwrite(stream, stream_idx, 1, fd);
        fclose(fd);
        fd = NULL;
    } while (0);
    if (fd) fclose(fd);
    if (stream) free(stream);
    return r;
}
