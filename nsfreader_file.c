#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "nsfreader_file.h"


// NSF File Reader
typedef struct nfr_ctx_s
{
    // super class
    nsfreader_t super;
    // Private fields
    FILE *fd;
    uint8_t *cache;
    uint32_t cache_size;
    uint32_t cache_data_length;
    uint32_t cache_offset;
#ifdef NFR_MEASURE_CACHE_PERFORMACE
    unsigned long long cache_hit;
    unsigned long long cache_miss;
#endif
} nfr_t;


static uint32_t read(nsfreader_t *self, uint8_t *out, uint32_t offset, uint32_t length)
{
    // Simplified implementation here considering most of the time we're requesting 1 byte
    nfr_t *ctx = (nfr_t*)self;
    if (length == 0)
        return 0;
    if (length > 1)
    {
        fseek(ctx->fd, offset, SEEK_SET);
        return (uint32_t)(fread(out, 1, length, ctx->fd));
    }
    if ((offset >= ctx->cache_offset) && (offset < ctx->cache_offset + ctx->cache_data_length))
    {
        // Cache hit
#ifdef NFR_MEASURE_CACHE_PERFORMACE
        ++(ctx->cache_hit);
#endif
        *out = ctx->cache[offset - ctx->cache_offset];
        return 1;
    }
    // Cache miss
#ifdef NFR_MEASURE_CACHE_PERFORMACE
    ++(ctx->cache_miss);
#endif
    fseek(ctx->fd, offset, SEEK_SET);
    ctx->cache_offset = offset;
    ctx->cache_data_length = (int32_t)fread(ctx->cache, 1, ctx->cache_size, ctx->fd);
    if (ctx->cache_data_length > 0)
    {
        *out = ctx->cache[0];
        return 1;
    }
    // Offset is out-of-bound
    return 0;
}


static uint32_t size(nsfreader_t *self)
{
    nfr_t *ctx = (nfr_t*)self;
    if (ctx && ctx->fd)
    {
        fseek(ctx->fd, 0, SEEK_END);
        size_t len = ftell(ctx->fd);
        return (uint32_t)len;
    }
    return 0;
}


nsfreader_t * nfr_create(const char *fn, uint32_t cache_size)
{
    FILE *fd = 0;
    nfr_t *ctx = 0;

    fd = fopen(fn, "rb");
    if (0 == fd)
        goto create_exit;

    ctx = (nfr_t*)malloc(sizeof(nfr_t));
    if (0 == ctx)
        goto create_exit;

    ctx->cache = (uint8_t*)malloc(cache_size);
    if (0 == ctx->cache)
        goto create_exit;

    ctx->fd = fd;
    ctx->cache_size = cache_size;
    ctx->cache_data_length = 0;
    ctx->cache_offset = 0;

    ctx->super.self = (nsfreader_t*)ctx;
    ctx->super.read = read;
    ctx->super.size = size;

#ifdef NFR_MEASURE_CACHE_PERFORMACE
    ctx->cache_hit = 0;
    ctx->cache_miss = 0;
#endif

    return (nsfreader_t*)ctx;

create_exit:
    if (ctx && ctx->cache)
        free(ctx->cache);
    if (ctx)
        free(ctx);
    if (fd)
        fclose(fd);
    return 0;
}


void nfr_destroy(nsfreader_t *nfr)
{
    nfr_t *ctx = (nfr_t*)nfr;
    if (0 == ctx)
        return;
    if (ctx->cache)
        free(ctx->cache);
    if (ctx->fd)
        fclose(ctx->fd);
    free(ctx);
}


#ifdef NFR_MEASURE_CACHE_PERFORMACE

void nfr_show_cache_status(nsfreader_t *nfr)
{
    nfr_t *ctx = (nfr_t*)nfr;
    NSF_PRINTF("Cache Status: (%llu/%llu), hit %.1f%%\n", ctx->cache_hit, ctx->cache_hit + ctx->cache_miss, (ctx->cache_hit * 100.0f) / (ctx->cache_hit + ctx->cache_miss));
}

#endif
