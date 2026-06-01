#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "canvas.h"

void render_text(stbtt_fontinfo* font, unsigned char* canvas, int canvas_w, int panel_x, int panel_w, int panel_h, const char* text, float pixel_height) {
    if (!font || !canvas || !text)
    {
        printf("render_text: null input\n");
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(font, pixel_height);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(font, &ascent, &descent, &lineGap);
    int line_height = (int)((ascent - descent + lineGap) * scale);

    int lines = 1;
    int maxw = 0;
    int curw = 0;

    for (int i = 0; text[i]; i++)
    {
        if (text[i] == '\n')
        {
            if (curw > maxw)
                maxw = curw;
            curw = 0;
            lines++;
            continue;
        }

        if (text[i] == '<')
        {
            while (text[i] && text[i] != '>')
                i++;
            continue;
        }

        int glyph = stbtt_FindGlyphIndex(font, text[i]);
        int ax, lsb;
        stbtt_GetGlyphHMetrics(font, glyph, &ax, &lsb);
        curw += (int)(ax * scale);
    }

    if (curw > maxw)
        maxw = curw;

    int block_h = lines * line_height;
    int start_x = panel_x + (panel_w - maxw) / 2;
    int start_y = (panel_h - block_h) / 2;

    int cursor_x = start_x;
    int cursor_y = start_y;
    int color = 0;

    for (int i = 0; text[i]; i++)
    {
        if (text[i] == '\n')
        {
            cursor_x = start_x;
            cursor_y += line_height;
            continue;
        }

        if (text[i] == '<')
        {
            if (strncmp(&text[i], "<red>", 5) == 0)
            {
                color = 1;
                while (text[i] && text[i] != '>') i++;
                continue;
            }

            if (strncmp(&text[i], "</red>", 6) == 0)
            {
                color = 0;
                while (text[i] && text[i] != '>') i++;
                continue;
            }

            while (text[i] && text[i] != '>') i++;
            continue;
        }

        int glyph = stbtt_FindGlyphIndex(font, text[i]);

        int ax, lsb;
        stbtt_GetGlyphHMetrics(font, glyph, &ax, &lsb);

        int x0, y0, x1, y1;
        stbtt_GetGlyphBitmapBox(font, glyph, scale, scale, &x0, &y0, &x1, &y1);

        int bw = x1 - x0;
        int bh = y1 - y0;

        if (bw <= 0 || bh <= 0)
        {
            cursor_x += (int)(ax * scale);
            continue;
        }

        unsigned char* bmp = malloc(bw * bh);

        if (!bmp)
        {
            printf("render_text: malloc failed for glyph\n");
            return;
        }

        stbtt_MakeGlyphBitmap(font, bmp, bw, bh, bw, scale, scale, glyph);

        int dx = cursor_x + (int)(lsb * scale);
        int dy = cursor_y + y0;

        for (int y = 0; y < bh; y++)
        {
            for (int x = 0; x < bw; x++)
            {
                unsigned char v = bmp[y * bw + x];
                if (!v) continue;

                int px = dx + x;
                int py = dy + y;

                if (px < 0 || py < 0 || px >= canvas_w) continue;

                int idx = (py * canvas_w + px) * 4;

                if (color)
                {
                    canvas[idx + 0] = 255;
                    canvas[idx + 1] = 80;
                    canvas[idx + 2] = 80;
                }
                else
                {
                    canvas[idx + 0] = v;
                    canvas[idx + 1] = v;
                    canvas[idx + 2] = v;
                }

                canvas[idx + 3] = 255;
            }
        }

        free(bmp);
        cursor_x += (int)(ax * scale);
    }
}

void render_canvas(int width, int height, const char* cover_path, const char* ttf_path, const char* text, const char* album, const char* out_base_name) {
    if (!cover_path || !ttf_path || !text || !album || !out_base_name)
    {
        printf("render_canvas: null argument\n");
        return;
    }

    unsigned char* canvas = malloc(width * height * 4);

    if (!canvas)
    {
        printf("render_canvas: canvas malloc failed\n");
        return;
    }

    for (int i = 0; i < width * height * 4; i += 4)
    {
        canvas[i + 0] = 0;
        canvas[i + 1] = 0;
        canvas[i + 2] = 0;
        canvas[i + 3] = 255;
    }

    int iw, ih, ic;
    unsigned char* img = stbi_load(cover_path, &iw, &ih, &ic, 4);
    if (!img)
    {
        printf("render_canvas: failed to load image %s\n", cover_path);
        free(canvas);
        return;
    }

    float scale = 1.0f;

    if (iw > 600 || ih > 600)
    {
        float sx = 600.0f / iw;
        float sy = 600.0f / ih;
        scale = sx < sy ? sx : sy;
    }

    int nw = (int)(iw * scale);
    int nh = (int)(ih * scale);

    unsigned char* resized = malloc(nw * nh * 4);
    if (!resized)
    {
        printf("render_canvas: resize malloc failed\n");
        free(img);
        free(canvas);
        return;
    }

    stbir_resize(img, iw, ih, 0, resized, nw, nh, 0,
                 STBIR_RGBA, STBIR_TYPE_UINT8,
                 STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT);

    int left_w = width / 2;
    int ox = (left_w - nw) / 2;
    int oy = (height - nh) / 2;

    for (int y = 0; y < nh; y++)
    {
        for (int x = 0; x < nw; x++)
        {
            int s = (y * nw + x) * 4;
            int d = ((y + oy) * width + (x + ox)) * 4;

            canvas[d + 0] = resized[s + 0];
            canvas[d + 1] = resized[s + 1];
            canvas[d + 2] = resized[s + 2];
            canvas[d + 3] = 255;
        }
    }

    FILE* f = fopen(ttf_path, "rb");
    if (!f)
    {
        printf("render_canvas: failed to open font %s\n", ttf_path);
        free(img);
        free(resized);
        free(canvas);
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char* font_buffer = malloc(size);

    if (!font_buffer)
    {
        printf("render_canvas: font malloc failed\n");
        fclose(f);
        free(img);
        free(resized);
        free(canvas);
        return;
    }

    fread(font_buffer, 1, size, f);
    fclose(f);

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, font_buffer,
        stbtt_GetFontOffsetForIndex(font_buffer, 0)))
    {
        printf("render_canvas: font init failed\n");
        free(font_buffer);
        free(img);
        free(resized);
        free(canvas);
        return;
    }

    // main text
    render_text(&font, canvas, width,
                width / 2,
                width / 2,
                height,
                text,
                60.0f);

    // album text
    render_text(&font, canvas, width,
                0,
                width / 2,
                height / 2 - 100,
                album,
                80.0f);

    char out[512];
    snprintf(out, 512, "%s.png", out_base_name);

    if (!stbi_write_png(out, width, height, 4, canvas, width * 4))
    {
        printf("render_canvas: failed to write image\n");
    }

    printf("saved frame: %s\n", out);

    free(canvas);
    free(resized);
    free(img);
    free(font_buffer);
}