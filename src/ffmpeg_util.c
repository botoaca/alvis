#include "ffmpeg_util.h"

int concat_audio_ffmpeg(char** files, int count, const char* out_base_name) {
    if (!files || count <= 0 || !out_base_name)
    {
        printf("concat_audio_ffmpeg: invalid arguments\n");
        return -1;
    }

    const char* list_path = "ffconcat_list.txt";

    FILE* f = fopen(list_path, "w");
    if (!f)
    {
        printf("concat_audio_ffmpeg: cannot create concat list file\n");
        return -1;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(f, "file '");

        for (const char* p = files[i]; *p; p++)
        {
            if (*p == '\'') fprintf(f, "'\\''");
            else fputc(*p, f);
        }

        fprintf(f, "'\n");
    }

    fclose(f);

    char output[512];
    snprintf(output, sizeof(output), "%s.flac", out_base_name);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -f concat -safe 0 -i %s -c:a flac %s",
        list_path,
        output
    );

    int ret = system(cmd);

    if (ret != 0) printf("concat_audio_ffmpeg: ffmpeg failed\n");

    remove(list_path);

    return ret;
}

int export_video_with_audio_ffmpeg(char** frame_names_base, int* durations, int count, const char* audio_path, const char* out_base_name) {
    if (count <= 0 || !frame_names_base || !durations || !audio_path || !out_base_name)
    {
        printf("export_video_with_audio_ffmpeg: invalid arguments\n");
        return 1;
    }

    const char* list_filename = "ffmpeg_frames.txt";
    FILE* f = fopen(list_filename, "w");
    if (!f)
    {
        printf("export_video_with_audio_ffmpeg: failed to create concat list file\n");
        return 1;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(f, "file '%s.png'\n", frame_names_base[i]);

        if (i < count - 1)
        {
            double seconds = durations[i] / 1000.0;
            fprintf(f, "duration %.6f\n", seconds);
        }
    }

    fprintf(f, "file '%s.png'\n", frame_names_base[count - 1]);

    fclose(f);

    char output_file[1024];
    snprintf(output_file, sizeof(output_file), "%s.mp4", out_base_name);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -y -f concat -safe 0 -i %s -i \"%s\" "
        "-fflags +genpts -vsync cfr "
        "-pix_fmt yuv420p -c:v libx264 -crf 18 -preset medium "
        "-c:a aac -shortest \"%s\"",
        list_filename,
        audio_path,
        output_file
    );

    int ret = system(cmd);
    if (ret != 0)
    {
        printf("export_video_with_audio_ffmpeg: ffmpeg failed with code %d\n", ret);
        remove(list_filename);
        return 1;
    }

    remove(list_filename);
    return 0;
}