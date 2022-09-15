#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include <cwalk.h>
#include <rogueutil.h>
#include "platform.h"
#include "nsf.h"
#include "nsfreader_file.h"
#include "nsfrip.h"

#define NSF2VGM_ERR_SUCCESS         0
#define NSF2VGM_ERR_OUTOFMEMORY     -1
#define NSF2VGM_ERR_IOERROR         -2
#define NSF2VGM_ERR_INVALIDCONFIG   -3
#define NSF2VGM_ERR_INVALIDNSF      -4

#define MAX_GAME_NAME       64
#define MAX_AUTHOR_NAME     128
#define MAX_RELEASE_DATE    16
#define MAX_TRACK_NAME      32

#define NSF_SAMPLE_RATE             44100
#define NSF_CACHE_SIZE              4096
#define NSF_DEFAULT_SAMPLE_LIMIT    (300 * SAMPLE_RATE) // 100 seconds
#define NSF_DEFAULT_MAX_RECORDS     10000000


#define PRINT_ERR(...) colorPrint(RED, -1, __VA_ARGS__)
#define PRINT_INF(...) colorPrint(BLUE, -1, __VA_ARGS__)



static void usage()
{
    PRINT_ERR("Usage: nsf2vgm config.json [track no]\n");
}


typedef struct convert_param_s
{
    const char *config_dir;             // directory where the json configuration is
    const char *nsf_path;               // absolute path of NSF file
    int index;                          // song index
    const char *track_name;
    const char *track_file_name;
    const char *override_out_dir;       // output dir if specified
    const char *override_game_name;     // game name if specified
    const char *override_authors;       // authors if specfiied
    const char *override_release_date;  // game release data if specified
} convert_param_t;
 

static int convert_nsf(convert_param_t *cp)
{
    int r = NSF2VGM_ERR_SUCCESS, t;

    char out_dir[MAX_PATH_NAME] = { '\0' };
    char game_name[MAX_GAME_NAME] = { '\0' };
    char authors[MAX_AUTHOR_NAME] = { '\0' };
    char release_date[MAX_RELEASE_DATE] = { '\0' };

    nsfreader_t *reader = NULL;
    nsfrip_t *rip = NULL;
    nsf_t *nsf = NULL;
    uint8_t *rom = NULL;
    uint16_t rom_len = 0;

    do
    {
        reader = nfr_create(nsf_path, NSF_CACHE_SIZE);
        if (NULL == reader)
        {
            r = NSF2VGM_ERR_IOERROR;
            PRINT_ERR("Failed to open NSF file \"%s\"\n", nsf_path);
            break;
        }
        nsf = nsf_create();
        if (!nsf)
        {
            r = NSF2VGM_ERR_OUTOFMEMORY;
            PRINT_ERR("Out of memory\n");
            break;
        }
        t = nsf_start_emu(nsf, reader, 10, NSF_SAMPLE_RATE, 1);
        if (NSF_ERR_SUCCESS != t)
        {
            r = NSF2VGM_ERR_INVALIDNSF;
            PRINT_ERR("File \"%s\" is not a valid NSF file\n", nsf_path);
            break;
        }
        PRINT_INF("Converting \"%s\"\n", nsf_path);

    } while (0);
    if (rom) free(rom);
    if (nsf) nsf_destroy(nsf);
    if (rip) nsfrip_destroy(rip);
    if (reader) nfr_destroy(reader);
    return r;
}


int process_config(const char *cf, int select)
{
    int r = NSF2VGM_ERR_SUCCESS;
    
    char base_dir[MAX_PATH_NAME] = { '\0' };
    char nsf_path[MAX_PATH_NAME] = { '\0' };

    char override_out_dir[MAX_PATH_NAME] = { '\0' };            // Allow config file to override output directory (if not specified, use config file dir + game name)
    char override_game_name[MAX_GAME_NAME] = { '\0' };          // Allow config file to override game name (if not specified, use game name inside nsf file)
    char override_authors[MAX_AUTHOR_NAME] = { '\0' };          // Allos config file to override game authors (if not specified, use author name inside nsf file)
    char override_release_date[MAX_RELEASE_DATE] = { '\0' };    // Allos config file to override release date (if not specified, try figure out from nsf file)
    char track_name[MAX_TRACK_NAME];
    char track_file_name[MAX_PATH_NAME];

    FILE *jfd = NULL;           // config file handle
    char *jstr = NULL;          // json string
    cJSON *config_json = NULL;  // json object of config file

    do
    {
        cwk_path_change_basename(cf, "", base_dir, MAX_PATH_NAME);

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
        fclose(jfd);    // jfd nolonger needed
        jfd = NULL;

        // parse file
        const cJSON *item = NULL;
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
        // Process requiured nsf_file value
        item = cJSON_GetObjectItemCaseSensitive(config_json, "nsf_file");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            if (cwk_path_is_relative(item->valuestring))
            {
                cwk_path_get_absolute(base_dir, item->valuestring, nsf_path, MAX_PATH_NAME);
            }
            else
            {
                strncpy(nsf_path, item->valuestring, MAX_PATH_NAME);
                nsf_path[MAX_PATH_NAME - 1] = '\0';
            }
        }
        else
        {
            NSF_PRINTERR("No valid nsf_file found.\n");
            r = NSF2VGM_ERR_INVALIDCONFIG;
            break;
        }
        NSF_PRINTDBG("NSF file: %s\n", nsf_path);
        // Process "game_name" value, Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "game_name");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            strncpy(override_game_name, item->valuestring, MAX_GAME_NAME);
            override_game_name[MAX_GAME_NAME - 1] = '\0';
            NSF_PRINTDBG("Override game name: %s\n", override_game_name);
        }
        // Process "out_dir". Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "out_dir");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            if (cwk_path_is_relative(item->valuestring))
            {
                cwk_path_get_absolute(base_dir, item->valuestring, override_out_dir, MAX_PATH_NAME);
            }
            else
            {
                strncpy(override_out_dir, item->valuestring, MAX_PATH_NAME);
                override_out_dir[MAX_PATH_NAME - 1] = '\0';
            }
            NSF_PRINTDBG("Override output dir: %s\n", override_out_dir);
        }
        // Process "authors" value. Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "authors");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            strncpy(override_authors, item->valuestring, MAX_AUTHOR_NAME);
            override_authors[MAX_AUTHOR_NAME - 1] = '\0';
            NSF_PRINTDBG("Override authors: %s\n", override_authors);
        }
        // Process "release_date" value. Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "release_date");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            strncpy(override_release_date, item->valuestring, MAX_RELEASE_DATE);
            override_release_date[MAX_RELEASE_DATE - 1] = '\0';
            NSF_PRINTDBG("Override release date: %s\n", override_release_date);
        }
        // Iteration on tracks
        const cJSON *track = NULL;
        const cJSON *tracks = NULL;
        tracks = cJSON_GetObjectItemCaseSensitive(config_json, "tracks");
        cJSON_ArrayForEach(track, tracks)
        {
            int index = -1;
            const cJSON *index_json = cJSON_GetObjectItemCaseSensitive(track, "index");
            if (cJSON_IsNumber(index_json))
            {
                index = index_json->valueint;
            }
            else if (cJSON_IsString(index_json) && (index_json->valuestring) != NULL && (index_json->valuestring[0] != '\0'))
            {
                index = atoi(index_json->valuestring);
            }
            if (index != -1)
            {
                if (select == 0 || index == select)
                {
                    NSF_PRINTDBG("Process track %d, ", index);
                    track_name[0] = '\0';
                    track_file_name[0] = '\0';
                    item = cJSON_GetObjectItemCaseSensitive(track, "name");
                    if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
                    {
                        strncpy(track_name, item->valuestring, MAX_TRACK_NAME);
                        track_name[MAX_TRACK_NAME - 1] = '\0';
                    }
                    // If file_name is specified, use it as output file name.
                    // If file_name is empty but name is specified, use "dd trackname.vgm" as output file name.
                    // Otherwise, use "Track dd" as track name and "Track dd.vgm" as output file name.
                    item = cJSON_GetObjectItemCaseSensitive(track, "file_name");
                    if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
                    {
                        strncpy(track_file_name, item->valuestring, MAX_PATH_NAME);
                        track_file_name[MAX_PATH_NAME - 1] = '\0';
                    }
                    else if (track_name[0] != '\0')
                    {
                        snprintf(track_file_name, MAX_PATH_NAME, "%02d %s.vgm", index, track_name);
                        track_file_name[MAX_PATH_NAME - 1] = '\0';
                    }
                    else
                    {
                        snprintf(track_name, MAX_TRACK_NAME, "Track %02d", index);
                        track_name[MAX_TRACK_NAME - 1] = '\0';
                        snprintf(track_file_name, MAX_PATH_NAME, "Track %02d.vgm", index);
                        track_file_name[MAX_PATH_NAME - 1] = '\0';
                    }
                    NSF_PRINTDBG("Track name \"%s\", save as \"%s\"\n", track_name, track_file_name);
                    convert_param_t params = { 0 };
                    params.config_dir = base_dir;
                    params.nsf_path = nsf_path;
                    params.index = index;
                    params.track_name = track_name;
                    params.track_file_name = track_file_name;
                    if (override_out_dir[0]) params.override_out_dir = override_out_dir;
                    if (override_game_name[0]) params.override_game_name = override_game_name;
                    if (override_authors[0]) params.override_authors = override_authors;
                    if (override_release_date[0]) params.override_release_date = override_release_date;
                    convert_nsf(&params);
                }
            }
        }
        
    } while (0);
    if (config_json) cJSON_Delete(config_json);
    if (jstr != NULL) free(jstr);
    if (jfd != NULL) fclose(jfd);
    return r;
}



int main(int argc, const char *argv[])
{
    int r = 0;
    
    saveDefaultColor();

    if (argc < 2)
    {
        usage();
        return -1;
    }

    const char *cf = argv[1];   // config file
    char config[MAX_PATH_NAME];
    // If path of config file itself is relative, extend it to absolute path so we can resolve
    // other paths set in the config file.
    if (cwk_path_is_relative(cf))
    {
        char *cwd = getcwd(NULL, 0);
        cwk_path_get_absolute(cwd, cf, config, MAX_PATH_NAME);
        free(cwd);
        cf = config;
    }
   
    int select = 0;
    if (argc == 3)
        select = atoi(argv[2]);
    
    r = process_config(cf, select);
  
    anykey(NULL);
	resetColor();

    return r;
}