#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include <cwalk.h>
#include "platform.h"
#include "nsf.h"

#define NSF2VGM_ERR_SUCCESS         0
#define NSF2VGM_ERR_OUTOFMEMORY     -1
#define NSF2VGM_ERR_IOERROR         -2
#define NSF2VGM_ERR_INVALIDCONFIG   -3


#define MAX_GAME_NAME       64
#define MAX_AUTHOR_NAME     128
#define MAX_RELEASE_DATE    16
#define MAX_TRACK_NAME      32


static void usage()
{
    printf("Usage: nsf2vgm config.json [track no]\n");
}




int process_config(const char *cf, int select)
{
    int r = 0;
    
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
  
    return r;
}