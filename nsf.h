#ifndef __NSF_H__
#define __NSF_H__

#include <stdint.h>
#include <stdbool.h>
#include "blip_buf.h"
#include "platform.h"
#include "nesbus.h"
#include "nescpu.h"
#include "nesapu.h"
#include "nsfreader.h"


#ifdef __cplusplus
extern "C" {
#endif


#define NSF_ERR_SUCCESS          0
#define NSF_ERR_INVALIDPARAM    -1
#define NSF_ERR_OUTOFMEMORY     -2
#define NSF_ERR_UNSUPPORTED     -3
#define NSF_ERR_NOT_INITIALIZED -4
#define NSF_ERR_DUMPFAILED      -5


// Spec: https://wiki.nesdev.org/w/index.php/NSF
PACK(struct nsf_header_s
{
    uint8_t id[5];              // 00 NESM\x1A
    uint8_t version;            // 05 spec version
    uint8_t num_songs;          // 06 total num songs
    uint8_t start_song;         // 07 first song
    uint16_t load_addr;         // 08 loc to load code
    uint16_t init_addr;         // 0A init call address
    uint16_t play_addr;         // 0C play call address
    uint8_t song_name[32];      // 0E name of song
    uint8_t artist_name[32];    // 2E artist name
    uint8_t copyright[32];      // 4E copyright info
    uint16_t ntsc_speed;        // 6E playback speed (if NTSC)
    uint8_t bankswitch_info[8]; // 70 initial code banking
    uint16_t pal_speed;         // 78 playback speed (if PAL)
    uint8_t pal_ntsc_bits;      // 7A NTSC/PAL determination bits
    uint8_t ext_sound_type;     // 7B type of external sound gen
    uint8_t reserved[4];        // 7C reserved
});
typedef struct nsf_header_s nsf_header_t;

typedef void (*apu_read_rom_cb)(uint16_t addr, void* param);
typedef void (*apu_write_reg_cb)(uint16_t addr, uint8_t val, void* param);

typedef struct nsf_s
{
    // NSF file
    nsf_header_t *header;
    uint32_t music_offset;
    uint32_t music_length;
    // Music data access
    nsfreader_t* reader;
    // NSF memory
    bool bank_switched;
    uint32_t bank[8];
    bool format;    // false - NTSC; true - PAL
    // Emulator
    nesbus_t *bus;
    nescpu_t *cpu;
    nesapu_t *apu;
    uint8_t *ram1;
    uint8_t *ram2;
    // Clock
    uint32_t cycles;
    uint32_t clock_rate;
    // Sampling
    unsigned long total_samples;
    uint32_t output_sample_rate;
    uint8_t oversample;
    // APU sample control
    uint32_t apu_sample_rate;
    uint32_t cycles_per_apu_sample;
    uint32_t next_apu_sample_cycle;
    int32_t apu_sample_cycle_error;
    int32_t accumulated_apu_sample_cycle_error;
    // Playback control
    uint32_t playback_rate;
    uint32_t cycles_per_playback;
    uint32_t next_playback_cycle;
    int32_t playback_cycle_error;
    int32_t accumulated_playback_cycle_error;
    // Blip
    uint16_t blip_buffer_size;
    blip_buffer_t* blip;
    int16_t blip_last_sample;
    // Slient detection
    bool silent;
    bool silence_detection;
    unsigned int slient_sample_target;
    unsigned int slient_sample_count;
    // APU sniffing
    bool sniff_enabled;
    apu_read_rom_cb sniff_apu_read_rom;
    apu_write_reg_cb sniff_write_apu_reg;
    void *sniff_param;
} nsf_t;


nsf_t* nsf_create();
void nsf_destroy(nsf_t* ctx);

int nsf_start_emu(nsf_t* ctx, nsfreader_t* reader, uint16_t max_sample_count, uint32_t sample_rate, uint8_t oversample);

void nsf_stop_emu(nsf_t *ctx);
int nsf_init_song(nsf_t *ctx, uint8_t song);
int nsf_get_samples(nsf_t *ctx, uint16_t count, int16_t* samples);
int nsf_enable_slience_detect(nsf_t *ctx, unsigned int samples);
bool nsf_silence_detected(nsf_t *ctx);
void nsf_enable_apu_sniffing(nsf_t *c, bool enable, apu_read_rom_cb read, apu_write_reg_cb write, void *param);
int nsf_dump_rom(nsf_t *ctx, int16_t addr, int16_t len, uint8_t *buf);

#ifdef __cplusplus
}
#endif


#endif