#ifndef CANVAS_H
#define CANVAS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
#include "stb_truetype.h"

void render_text(stbtt_fontinfo* font, unsigned char* canvas, int canvas_w, int panel_x, int panel_w, int panel_h, const char* text, float pixel_height);
void render_canvas(int width, int height, const char* cover_path, const char* ttf_path, const char* text, const char* album, const char* out_base_name);

#endif