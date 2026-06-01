#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <FLAC/metadata.h>

#include "flac_util.h"
#include "ffmpeg_util.h"
#include "canvas.h"

#define MAX_SONGS 50

void ms_to_time_string(long ms, char* buf, size_t buf_size)
{
    long total_seconds = ms / 1000;
    long minutes = total_seconds / 60;
    long seconds = total_seconds % 60;

    snprintf(buf, buf_size, "%02ld:%02ld", minutes, seconds);
}

int main(int argc, char** argv) {
    if (argc != 3)
    {
        printf("usage: ./alvis <music folder> <ttf file>\n");
        return 1;
    }

    char* music_path = argv[1];
    char* font_path = argv[2];

    DIR* music_dir = opendir(music_path);
    if (!music_dir)
    {
        perror("opendir failed");
        return 1;
    }

    struct dirent* music_entry;

    char** music_filenames = malloc(MAX_SONGS * sizeof(char*));
    int music_filenames_len = 0;

    while ((music_entry = readdir(music_dir)) != NULL)
    {
        char* filename = music_entry->d_name;
        if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) continue;

        music_filenames[music_filenames_len] = malloc(strlen(music_path) + strlen(filename) + 2);

        sprintf(music_filenames[music_filenames_len], "%s/%s", music_path, filename);

        music_filenames_len++;
    }

    closedir(music_dir);

// SORT BY TRACK NUMBER
    struct SongFile
    {
        char* path;
        int track_number;
        int duration;
    };

    struct SongFile songs[MAX_SONGS];

    for (int i = 0; i < music_filenames_len; i++)
    {
        struct Track t = read_flac_track(music_filenames[i]);

        songs[i].path = music_filenames[i];
        songs[i].track_number = t.track_number;
        songs[i].duration = t.duration;

        free(t.title);
        free(t.artist);
        free(t.album);
    }

    int cmp(const void* a, const void* b) {
        const struct SongFile* sa = (const struct SongFile*)a;
        const struct SongFile* sb = (const struct SongFile*)b;
        return sa->track_number - sb->track_number;
    }

    qsort(songs, music_filenames_len, sizeof(struct SongFile), cmp);

    for (int i = 0; i < music_filenames_len; i++)
    {
        music_filenames[i] = songs[i].path;
    }
// SORT BY TRACK NUMBER

    int cover_type = -1;
    if (extract_flac_cover_art(music_filenames[0], "cover", &cover_type) != 0)
        return 1;

    char* cover_filename = (cover_type == 0) ? "cover.jpg" : "cover.png";

    char** frame_names = malloc(MAX_SONGS * sizeof(char*));
    int* durations = malloc(MAX_SONGS * sizeof(int));

    char* caption = calloc(1, 8192);
    int timestamp = 0;

    for (int i = 0; i < music_filenames_len; i++)
    {
        struct Track cur_track = read_flac_track(music_filenames[i]);

        char artist_and_album[256];
        snprintf(artist_and_album, sizeof(artist_and_album), "%s - %s", cur_track.artist, cur_track.album);

        char tracklist[4096];
        int pos = 0;
        tracklist[0] = '\0';

        for (int c = 0; c < music_filenames_len; c++)
        {
            struct Track t = read_flac_track(music_filenames[c]);

            char line[128];

            if (c == i) snprintf(line, sizeof(line), "<red>%d. %s</red>\n", t.track_number, t.title);
            else snprintf(line, sizeof(line), "%d. %s\n", t.track_number, t.title);

            int len = strlen(line);
            if ((size_t)(pos + len) < sizeof(tracklist))
            {
                memcpy(tracklist + pos, line, len);
                pos += len;
                tracklist[pos] = '\0';
            }

            free(t.title);
            free(t.artist);
            free(t.album);
        }

        char frame_name[64];
        snprintf(frame_name, sizeof(frame_name), "frame_%d", i);

        render_canvas(
            1920,
            1080,
            cover_filename,
            font_path,
            tracklist,
            artist_and_album,
            frame_name
        );

        frame_names[i] = strdup(frame_name);
        durations[i] = cur_track.duration;

        char line[256];
        char start_buf[32];
        char dur_buf[32];

        ms_to_time_string(timestamp, start_buf, sizeof(start_buf));
        ms_to_time_string(cur_track.duration, dur_buf, sizeof(dur_buf));
        snprintf(line, sizeof(line),
                "%d. %s (starts @ %s, lasts %s)\n",
                cur_track.track_number,
                cur_track.title,
                start_buf,
                dur_buf);

        strncat(caption, line, 8192 - strlen(caption) - 1);

        timestamp += cur_track.duration;

        free(cur_track.title);
        free(cur_track.artist);
        free(cur_track.album);
    }
    
    concat_audio_ffmpeg(music_filenames, music_filenames_len, "audio");
    export_video_with_audio_ffmpeg(frame_names, durations, music_filenames_len, "audio.flac", "video");
    
    printf("\n%s", caption);

    return 0;
}