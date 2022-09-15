#pragma once


#ifdef __GNUC__
# define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
# define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#include<stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


#define NSF_PRINTF(...) printf(__VA_ARGS__)
#define NSF_PRINTDBG(...) fprintf(stderr, __VA_ARGS__)
#define NSF_PRINTERR(...) fprintf(stderr, __VA_ARGS__)


#ifdef __cplusplus
}
#endif