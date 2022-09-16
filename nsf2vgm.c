#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <cJSON.h>
#include <cwalk.h>
#include "platform.h"
#include "ansicon.h"
#include "nsf.h"
#include "nsfreader_file.h"
#include "nsfrip.h"

#define NSF2VGM_ERR_SUCCESS             0
#define NSF2VGM_ERR_OUTOFMEMORY         -1
#define NSF2VGM_ERR_IOERROR             -2
#define NSF2VGM_ERR_INVALIDCONFIG       -3
#define NSF2VGM_ERR_INVALIDNSF          -4
#define NSF2VGM_ERR_INSUFFICIENT_DATA   -5

#define MAX_GAME_NAME       64
#define MAX_AUTHOR_NAME     128
#define MAX_RELEASE_DATE    16
#define MAX_TRACK_NAME      32

#define NSF_SAMPLE_RATE             44100
#define NSF_CACHE_SIZE              4096
#define NSF_DEFAULT_SAMPLE_LIMIT    (300 * SAMPLE_RATE) // 100 seconds
#define NSF_DEFAULT_MAX_RECORDS     10000000


#define PRINT_ERR(...) do { printf("%s", ANSI_RED); printf(__VA_ARGS__); } while (0)
#define PRINT_INF(...) do { printf("%s", ANSI_LIGHTBLUE); printf(__VA_ARGS__); } while (0)



static void usage()
{
    PRINT_ERR("%s", "Usage: nsf2vgm config.json [track no]\n");
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
    char vgm_path[MAX_PATH_NAME] = { '\0' };
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
        reader = nfr_create(cp->nsf_path, NSF_CACHE_SIZE);
        if (NULL == reader)
        {
            r = NSF2VGM_ERR_IOERROR;
            PRINT_ERR("Failed to open NSF file \"%s\"\n", cp->nsf_path);
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
            PRINT_ERR("File \"%s\" is not a valid NSF file\n", cp->nsf_path);
            break;
        }
        // check if index is valid
        if (cp->index > nsf->header->num_songs)
        {
            r = NSF2VGM_ERR_INVALIDCONFIG;
            PRINT_ERR("Track #%d not found in NSF file\n", cp->index);
            break;
        }
        // check meta inside nsf header vs. overrided parameters
        if (cp->override_game_name)
        {
            strncpy(game_name, cp->override_game_name, MAX_GAME_NAME);
            game_name[MAX_GAME_NAME - 1] = '\0';
        }
        else
        {
            strncpy(game_name, (const char*)nsf->header->song_name, MAX_GAME_NAME);
            game_name[MAX_GAME_NAME - 1] = '\0';
        }
        if (!game_name[0])
        {
            r = NSF2VGM_ERR_INSUFFICIENT_DATA;
            PRINT_ERR("The NSF file does not contain a game name, please specify in config json\n");
            break;
        }
        // with game name we can decide output dir
        if (cp->override_out_dir)
        {
            strncpy(out_dir, cp->override_out_dir, MAX_PATH_NAME);
            out_dir[MAX_PATH_NAME - 1] = '\0';
        }
        else
        {
            cwk_path_get_absolute(cp->config_dir, game_name, out_dir, MAX_PATH_NAME);
        }
        // output VGM file
        cwk_path_get_absolute(out_dir, cp->track_file_name, vgm_path, MAX_PATH_NAME);
        // authors
        if (cp->override_authors)
        {
            strncpy(authors, cp->override_authors, MAX_AUTHOR_NAME);
            authors[MAX_AUTHOR_NAME - 1] = '\0';
        }
        else if (nsf->header->artist_name[0])
        {
            strncpy(authors, (const char *)nsf->header->artist_name, MAX_AUTHOR_NAME);
            authors[MAX_AUTHOR_NAME - 1] = '\0';
        }
        else
        {
            strcpy(authors, "N/A");
        }
        // Release date (GD3 requres yyyy/mm/dd or yyyy/mm or yyyy)
        if (cp->override_release_date)
        {
            strncpy(release_date, cp->override_release_date, MAX_RELEASE_DATE);
            release_date[MAX_RELEASE_DATE - 1] = '\0';
        }
        else if (nsf->header->copyright[0])
        {
            // try infer release date (year) from copyright
            char *p = (char *)nsf->header->copyright;
            while (*p)
            {
                if (isdigit(*p))
                {
                    int val = (int)strtol(p, &p, 10);
                    if (val > 1900)
                    {
                        snprintf(release_date, MAX_RELEASE_DATE, "%d", val);
                    }
                    else if (val < 100)
                    {
                        snprintf(release_date, MAX_RELEASE_DATE, "%d", val + 1900);
                    }
                    break;
                }
                else
                {
                    ++p;
                }
            }
        }
        else
        {
            // default date
            strcpy(release_date, "1980/01/01");
        }
        // Meta collection done
        PRINT_INF("Source File:  %s\n", cp->nsf_path);
        PRINT_INF("Save to:      %s\n", vgm_path);
        PRINT_INF("Game name:    %s\n", game_name);
        PRINT_INF("Track %02d:     %s\n", cp->index, cp->track_name);
        PRINT_INF("Authors:      %s\n", authors);
        PRINT_INF("Release date: %s\n", release_date);
        
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
            PRINT_ERR("Failed to open \"%s\"\n", cf);
            break;
        }
        fseek(jfd, 0, SEEK_END);
        long sz = ftell(jfd);
        fseek(jfd, 0, SEEK_SET);
        jstr = malloc(sz + 1);
        if (NULL == jstr)
        {
            r = NSF2VGM_ERR_OUTOFMEMORY;
            PRINT_ERR("Out of memory\n");
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
                PRINT_ERR("Config file error before: %s\n", error_ptr);
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
            PRINT_ERR("No valid nsf_file found.\n");
            r = NSF2VGM_ERR_INVALIDCONFIG;
            break;
        }
        // Process "game_name" value, Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "game_name");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            strncpy(override_game_name, item->valuestring, MAX_GAME_NAME);
            override_game_name[MAX_GAME_NAME - 1] = '\0';
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
        }
        // Process "authors" value. Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "authors");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            strncpy(override_authors, item->valuestring, MAX_AUTHOR_NAME);
            override_authors[MAX_AUTHOR_NAME - 1] = '\0';
        }
        // Process "release_date" value. Leave empty if not specified.
        item = cJSON_GetObjectItemCaseSensitive(config_json, "release_date");
        if (cJSON_IsString(item) && (item->valuestring != NULL) && (item->valuestring[0] != '\0'))
        {
            strncpy(override_release_date, item->valuestring, MAX_RELEASE_DATE);
            override_release_date[MAX_RELEASE_DATE - 1] = '\0';
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
    
    ansicon_setup();

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
  
	ansicon_restore();

    return r;
}