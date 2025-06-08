#include "png.h"
#include <png.h>
#include <stdio.h>


void VRImage::setup(int w, int h, int c) {
    width = w;
    height = h;
    channels = c;
    pixels = vector<unsigned char>(c*w*h, 0);
}

void VRImage::resize(int w, int h) {
    int Nc = channels;
    vector<unsigned char> newPixs(Nc*w*h, 0);

    for (int y = 0; y < h; y++) {
        int Y = y * height / h;
        for (int x = 0; x < w; x++) {
            int X = x * width / w;
            for (int c = 0; c < Nc; c++) {
                newPixs[(y*w + x) *Nc + c] = pixels[(Y*width + X) *Nc + c];
            }
        }
    }

    pixels = newPixs;
    width = w;
    height = h;
}

void readPNG(string path, VRImage& img) {} // TODO

void writePNG(string path, VRImage& img) {
    int width = img.width;
    int height = img.height;
    unsigned char* pixels = &img.pixels[0];

    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) { perror("fopen"); return; }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) { fclose(fp); return; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_write_struct(&png_ptr, NULL); fclose(fp); return; }
    if (setjmp(png_jmpbuf(png_ptr))) { png_destroy_write_struct(&png_ptr, &info_ptr); fclose(fp); return; }

    png_init_io(png_ptr, fp);

    auto colorType = (img.channels == 4) ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB;

    // Set image info
    png_set_IHDR(
        png_ptr, info_ptr, width, height,
        8,                      // bit depth
        colorType,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );
    png_write_info(png_ptr, info_ptr);

    png_bytep row_pointers[height];
    for (int y = 0; y < height; ++y) {
        row_pointers[y] = pixels + (height - 1 - y) * width * img.channels; // flip y
    }

    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

