#ifndef FLAC_UTIL_H
#define FLAC_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FLAC/metadata.h>

struct Track {
    char* title;
    char* artist;
    char* album;
    int track_number;
    int duration;
};

struct Track read_flac_track(const char* filename);
int extract_flac_cover_art(const char* filename, const char* out_base_name, int* cover_type);

#endif