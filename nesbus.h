#pragma once

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#define BUS_OWNER_CPU   1
#define BUS_OWNER_APU   2

typedef struct nesbus_read_handler_s
{
    const char* tag;
    uint16_t lo, hi;
    void* cookie;
    bool (*fn)(uint16_t addr, uint8_t* rval, void *cookie, uint8_t owner);  // read function, return true if succeeded
} nesbus_read_handler_t;


typedef struct nesbus_write_handler_s
{
    const char* tag;
    uint16_t lo, hi;
    void* cookie;
    bool (*fn)(uint16_t addr, uint8_t val, void *cookie);  // write function, return true if succeeded
} nesbus_write_handler_t;



typedef struct nesbus_ctx_s
{
    nesbus_read_handler_t* read_table;
    int read_table_max, read_table_cur;
    nesbus_write_handler_t* write_table;
    int write_table_max, write_table_cur;
} nesbus_t;


nesbus_t* nesbus_create(int max_read, int max_write);
void nesbus_destroy(nesbus_t* ctx);
bool nesbus_add_read_handler(nesbus_t* ctx, const char* tag, uint16_t lo, uint16_t hi, bool (*handler)(uint16_t, uint8_t*, void*, uint8_t owner), void* cookie);
bool nesbus_add_write_handler(nesbus_t* ctx, const char* tag, uint16_t lo, uint16_t hi, bool (*handler)(uint16_t, uint8_t, void*), void* cookie);
void nesbus_clear_handlers(nesbus_t* ctx);
void nesbus_dump_handlers(nesbus_t* ctx);

uint8_t nesbus_read(nesbus_t* ctx, uint16_t address, uint8_t owner);
void nesbus_write(nesbus_t* ctx, uint16_t address, uint8_t value);



#ifdef __cplusplus
}
#endif
