#include <stdlib.h>
#include <memory.h>
#include "platform.h"
#include "nesbus.h"


nesbus_t* nesbus_create(int max_read, int max_write)
{
    nesbus_t* c = (nesbus_t *)malloc(sizeof(nesbus_t));
    if (0 == c)
        return 0;
    memset(c, 0, sizeof(nesbus_t));
    c->read_table = (nesbus_read_handler_t *)malloc(sizeof(nesbus_read_handler_t) * max_read);
    if (0 == c->read_table)
    {
        nesbus_destroy(c);
        return 0;
    }
    c->write_table = (nesbus_write_handler_t *)malloc(sizeof(nesbus_write_handler_t) * max_write);
    if (0 == c->write_table)
    {
        nesbus_destroy(c);
        return 0;
    }
    memset(c->read_table, 0, sizeof(nesbus_read_handler_t) * max_read);
    memset(c->write_table, 0, sizeof(nesbus_write_handler_t) * max_write);
    c->read_table_max = max_read;
    c->write_table_max = max_write;
    return c;
}


void nesbus_destroy(nesbus_t* c)
{
    if (c)
    {
        if (c->read_table)
            free(c->read_table);
        if (c->write_table)
            free(c->write_table);
        free(c);
    }
}


bool nesbus_add_read_handler(nesbus_t* c, const char* tag, uint16_t lo, uint16_t hi, bool (*handler)(uint16_t, uint8_t*, void*, uint8_t owner), void* cookie)
{
    if (0 == c)
        return false;
    if (c->read_table_cur >= c->read_table_max)
        return false;
    c->read_table[c->read_table_cur] = (nesbus_read_handler_t)
    {
        .tag = tag,
        .lo = lo,
        .hi = hi,
        .cookie = cookie,
        .fn = handler
    };
    ++(c->read_table_cur);
    return true;
}


bool nesbus_add_write_handler(nesbus_t* c, const char* tag, uint16_t lo, uint16_t hi, bool (*handler)(uint16_t, uint8_t, void*), void* cookie)
{
    if (0 == c)
        return false;
    if (c->write_table_cur >= c->write_table_max)
        return false;
    c->write_table[c->write_table_cur] = (nesbus_write_handler_t)
    {
        .tag = tag,
        .lo = lo,
        .hi = hi,
        .cookie = cookie,
        .fn = handler
    };
    ++(c->write_table_cur);
    return true;
}


void nesbus_clear_handlers(nesbus_t* c)
{
    if (0 == c)
        return;
    memset(c, 0, sizeof(nesbus_t));
}


void nesbus_dump_handlers(nesbus_t* c)
{
    int i;
    if (0 == c)
        return;
    NSF_PRINTF("NSF: bus read handlers:\n");
    for (i = 0; i < c->read_table_cur; ++i)
    {
        if (0 != c->read_table[i].fn)
        {
            if (0 != c->read_table[i].tag)
            {
                NSF_PRINTF("(%02d): 0x%04x--0x%04x (%s)\n", i, c->read_table[i].lo, c->read_table[i].hi, c->read_table[i].tag);
            }
            else
            {
                NSF_PRINTF("(%02d): 0x%04x--0x%04x (%p)\n", i, c->read_table[i].lo, c->read_table[i].hi, c->read_table[i].fn);
            }
        }
    }
    NSF_PRINTF("NSF: bus write handlers:\n");
    for (i = 0; i < c->write_table_cur; ++i)
    {
        if (c->write_table[i].fn != 0)
        {
            if (c->write_table[i].tag != 0)
            {
                NSF_PRINTF("(%02d): 0x%04x--0x%04x (%s)\n", i, c->write_table[i].lo, c->write_table[i].hi, c->write_table[i].tag);
            }
            else
            {
                NSF_PRINTF("(%02d): 0x%04x--0x%04x (%p)\n", i, c->write_table[i].lo, c->write_table[i].hi, c->write_table[i].fn);
            }
        }
    }
}


uint8_t nesbus_read(nesbus_t* c, uint16_t addr, uint8_t owner)
{
    int i;
    bool r = false;
    uint8_t val = 0, tval;
    
    if (0 == c)
        return 0;

    // loop through all reader functions.
    // First reader returns true will set the output value
    for (i = 0; i < c->read_table_cur; ++i)
    {
        if (c->read_table[i].fn != 0)
        {
            if ((addr >= c->read_table[i].lo) && (addr <= c->read_table[i].hi))
            {
                if (c->read_table[i].fn(addr, &tval, c->read_table[i].cookie, owner))
                {
                    if (!r)
                    {
                        val = tval;
                        r = true;
                    }
                }
            }
        }
    }
    if (!r)  // no handler found
    {
#ifndef NESBUS_SUPPRESS_ERROR_MESSAGE
        NSF_PRINTERR("NSF: no read handler found for address 0x%x\n", addr);
#endif
    }
    return val;
}


void nesbus_write(nesbus_t* c, uint16_t addr, uint8_t val)
{
    int i;
    bool r = false;

    if (0 == c)
        return;

    // loop through all writer functions.
    for (i = 0; i < c->write_table_cur; ++i)
    {
        if (c->write_table[i].fn != 0)
        {
            if ((addr >= c->write_table[i].lo) && (addr <= c->write_table[i].hi))
            {
                r = c->write_table[i].fn(addr, val, c->write_table[i].cookie) | r;
            }
        }
    }
    if (!r) // no handler found
    {
#ifndef NESBUS_SUPPRESS_ERROR_MESSAGE
        NSF_PRINTERR("NSF: no write handler found for address 0x%x\n", addr);
#endif
    }
}


