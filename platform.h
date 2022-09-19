#pragma once


#if defined(__GNUC__)
# define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#elif defined(_WIN32)
# ifdef _MSC_VER
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
# else
#  error "MSVC required on Windows"
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
# include <direct.h>
# define getcwd _getcwd
# define strcasecmp _strcmpi
# define mkdir(d,m) _mkdir(d)
#else
# include <unistd.h>
# include <sys/stat.h>
#endif

#define MAX_PATH_NAME   256

#define NSF_PRINTF(...) printf(__VA_ARGS__)
#define NSF_PRINTDBG(...) fprintf(stderr, __VA_ARGS__)
#define NSF_PRINTERR(...) fprintf(stderr, __VA_ARGS__)

