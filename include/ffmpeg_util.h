#ifndef FFMPEG_UTIL_H
#define FFMPEG_UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

int concat_audio_ffmpeg(char** files, int count, const char* out_base_name);
int export_video_with_audio_ffmpeg(char** frame_names_base, int* durations, int count, const char* audio_path, const char* out_base_name);

#endif