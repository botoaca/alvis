#include "flac_util.h"

struct Track read_flac_track(const char* filename) {
    struct Track track = {0};
    track.duration = 0;
    track.track_number = 0;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
    if (!chain) return track;

    if (!FLAC__metadata_chain_read(chain, filename))
    {
        FLAC__metadata_chain_delete(chain);
        return track;
    }

    FLAC__Metadata_Iterator *it = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(it, chain);

    do
    {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(it);

        // STREAMINFO
        if (block->type == FLAC__METADATA_TYPE_STREAMINFO)
        {
            const FLAC__StreamMetadata_StreamInfo *si = &block->data.stream_info;

            if (si->sample_rate > 0)
            {
                double _duration = (double)(si->total_samples / si->sample_rate);
                track.duration = (int)_duration * 1000.0 + 0.5;
            }
        }

        // VORBIS COMMENTS
        else if (block->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
        {
            const FLAC__StreamMetadata_VorbisComment *vc = &block->data.vorbis_comment;

            for (unsigned i = 0; i < vc->num_comments; i++)
            {
                const char *entry = (const char*)vc->comments[i].entry;

                if (strncasecmp(entry, "TITLE=", 6) == 0)
                {
                    free(track.title);
                    track.title = strdup(entry + 6);
                }

                else if (strncasecmp(entry, "ARTIST=", 7) == 0)
                {
                    free(track.artist);
                    track.artist = strdup(entry + 7);
                }

                else if (strncasecmp(entry, "ALBUM=", 6) == 0)
                {
                    free(track.album);
                    track.album = strdup(entry + 6);
                }

                else if (strncasecmp(entry, "TRACKNUMBER=", 12) == 0)
                {
                    track.track_number = atoi(entry + 12);
                }
            }
        }

    }
    while (FLAC__metadata_iterator_next(it));

    FLAC__metadata_iterator_delete(it);
    FLAC__metadata_chain_delete(chain);

    return track;
}

int extract_flac_cover_art(const char* filename, const char* out_base_name, int* cover_type) {
    FLAC__Metadata_Chain* chain = FLAC__metadata_chain_new();
    if (!chain) return 1;

    if (!FLAC__metadata_chain_read(chain, filename))
    {
        FLAC__metadata_chain_delete(chain);
        return 1;
    }

    FLAC__Metadata_Iterator* it = FLAC__metadata_iterator_new();
    FLAC__metadata_iterator_init(it, chain);

    int ret = 1;

    do
    {
        FLAC__StreamMetadata* block = FLAC__metadata_iterator_get_block(it);

        if (block->type == FLAC__METADATA_TYPE_PICTURE)
        {
            const FLAC__StreamMetadata_Picture* pic = &block->data.picture;

            const char* ext = "bin";

            if (pic->mime_type)
            {
                if (strstr(pic->mime_type, "jpeg"))
                {
                    ext = "jpg";
                    *cover_type = 0;
                }
                else if (strstr(pic->mime_type, "png"))
                {
                    ext = "png";
                    *cover_type = 1;
                }
            }

            char outname[256];
            snprintf(outname, sizeof(outname), "%s.%s", out_base_name, ext);

            FILE* f = fopen(outname, "wb");
            if (!f)
            {
                perror("fopen");
                break;
            }

            fwrite(pic->data, 1, pic->data_length, f);
            fclose(f);

            ret = 0;
            break;
        }

    }
    while (FLAC__metadata_iterator_next(it));

    FLAC__metadata_iterator_delete(it);
    FLAC__metadata_chain_delete(chain);

    return ret;
}