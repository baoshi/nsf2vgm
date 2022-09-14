#include <stdio.h>
#include "cJSON.h"


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
    char* json = argv[1];
    FILE *jf = NULL;

    do
    {
        jf = fopen(argv[1], "rb");
        if (NULL == jf)
        {
            fprintf(stderr, "Failed to open \"%s\"\n", json);
            break;
        }

    } while (0);
    if (jf != NULL) fclose(jf);
    return 0;
}