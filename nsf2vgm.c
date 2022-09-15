#include <stdio.h>
#include <cJSON.h>
#include "platform.h"


void usage()
{
    printf("Usage: nsf2vgm config.json [track no]\n");
}


int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        usage();
        return -1;
    }
    const char *cf = argv[1]; // config file
    FILE *jf = NULL;

    do
    {
        jf = fopen(cf, "rb");
        if (NULL == jf)
        {
            fprintf(stderr, "Failed to open \"%s\"\n", cf);
            break;
        }


    } while (0);
    if (jf != NULL) fclose(jf);
    return 0;
}