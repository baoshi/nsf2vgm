#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "nesbus.h"

typedef struct nescpu_s
{
    uint16_t PC;                        // register
    uint8_t SP, A, X, Y, STATUS;        // register
    nesbus_t* bus;                      // Bus
    uint16_t abs_addr, rel_addr;        // calculated address
    uint8_t opcode;                     // opcode
    bool add_cycle_op, add_cycle_addr;  // if the instruction will add additional cycle
    bool jammed;                        // if received JAM instruction
    uint8_t cycles;                     // keep track how many cycles left for current instruction
} nescpu_t;

nescpu_t * nescpu_create();
void nescpu_destroy(nescpu_t *ctx);
void nescpu_attach_bus(nescpu_t* ctx, nesbus_t* bus);
void nescpu_reset(nescpu_t* ctx, bool reset_to_fffc);
void nescpu_irq(nescpu_t* ctx);
void nescpu_nmi(nescpu_t* ctx);
bool nescpu_clock(nescpu_t* ctx);
bool nescpu_is_completed(nescpu_t* ctx);
bool nescpu_is_jammed(nescpu_t* ctx);
void nescpu_unjam(nescpu_t* ctx);
void nescpu_dump(nescpu_t* ctx);
void nescpu_set_pc(nescpu_t* ctx, uint16_t pc);
void nescpu_set_a(nescpu_t* ctx, uint8_t a);
void nescpu_set_x(nescpu_t* ctx, uint8_t x);
void nescpu_set_y(nescpu_t* ctx, uint8_t y);
void nescpu_set_status(nescpu_t* ctx, uint8_t status);
void nescpu_set_sp(nescpu_t* ctx, uint8_t sp);
void nescpu_skip_cycles(nescpu_t* ctx, int cycles);

#ifdef __cplusplus
}
#endif

