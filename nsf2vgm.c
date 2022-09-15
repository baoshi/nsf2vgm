#include <stdio.h>
#include <cJSON.h>
#include "platform.h"

#define NSF2VGM_ERR_SUCCESS         0
#define NSF2VGM_ERR_OUTOFMEMORY     -1
#define NSF2VGM_ERR_IOERROR         -2
#define NSF2VGM_ERR_INVALIDCONFIG   -3

static void usage()
{
    printf("Usage: nsf2vgm config.json [track no]\n");
}




int process_config(const char *jstr, int index)
{
    int r = 0;
    cJSON *config_json = NULL;
    do
    {
        cJSON *config_json = cJSON_Parse(jstr);
        if (NULL == config_json)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL)
            {   
                NSF_PRINTERR("Config file error before: %s\n", error_ptr);
            }
            r = NSF2VGM_ERR_INVALIDCONFIG;
            break;
        }
        const cJSON *nsf_file = NULL;
        nsf_file = cJSON_GetObjectItemCaseSensitive(config_json, "nsf_file");
        if (cJSON_IsString(nsf_file) && (nsf_file->valuestring != NULL))
        {
            NSF_PRINTF("Checking file %s\n", nsf_file->valuestring);
        }
        else
        {
            NSF_PRINTERR("No valid NSF input found.\n");
            r = NSF2VGM_ERR_INVALIDCONFIG;
            break;
        }
        

    } while (0);
    if (config_json) cJSON_Delete(config_json);
    return r;
}


int main(int argc, const char *argv[])
{
    int r = 0;
    if (argc < 2)
    {
        usage();
        return -1;
    }
    const char *cf = argv[1];   // config file
    FILE *jfd = NULL;           // config file handle
    char *jstr = NULL;          // json string
    int index = 0;

    if (argc == 2)
    {
        index = atoi(argv[2]);
    }
    
    do
    {
        jfd = fopen(cf, "rb");
        if (NULL == jfd)
        {
            r = NSF2VGM_ERR_IOERROR;
            NSF_PRINTERR("Failed to open \"%s\"\n", cf);
            break;
        }
        fseek(jfd, 0, SEEK_END);
        long sz = ftell(jfd);
        fseek(jfd, 0, SEEK_SET);
        jstr = malloc(sz + 1);
        if (NULL == jstr)
        {
            r = NSF2VGM_ERR_OUTOFMEMORY;
            NSF_PRINTERR("Out of memory\n");
            break;
        }
        fread(jstr, sz, 1, jfd);
        jstr[sz] = '\0';
        fclose(jfd);
        jfd = NULL;
        r = process_config(jstr, index);


    } while (0);

    if (jstr != NULL) free(jstr);
    if (jfd != NULL) fclose(jfd);
    return r;
}