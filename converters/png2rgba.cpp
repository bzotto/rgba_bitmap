// Converter from PNG to the RGBA bitmap file format:
// https://github.com/bzotto/rgba_bitmap
//
// Compile using e.g:
// $ g++ png2rgba.cpp -O3 -lpng -o png2rgba
//
// Adapted by Roar Lauritzsen from code found here:
// https://gist.github.com/niw/5963798
// Modified by Yoshimasa Niwa from code originally found here:
// http://zarb.org/~gc/html/libpng.html
//
// Original copyright notice:
// ------------------------------------------------------------
// Copyright 2002-2010 Guillaume Cottenceau.
//
// This software may be freely redistributed under the terms
// of the X11 license.
// ------------------------------------------------------------
   
#include <arpa/inet.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <png.h>
#include <stdexcept>

using namespace std;

void read_png_file(char* filename, uint32_t& width, uint32_t& height, uint8_t*& rgba)
{
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    throw runtime_error{"Failed to open file: " + string{filename}};
  }

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (setjmp(png_jmpbuf(png))) {
    throw runtime_error{"Error reading PNG image: " + string{filename}};
  }
  png_init_io(png, fp);

  png_infop info = png_create_info_struct(png);
  png_read_info(png, info);
  width = png_get_image_width(png, info);
  height = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth = png_get_bit_depth(png, info);

  // Convert to RGBA
  if (bit_depth == 16) {
    png_set_strip_16(png);
  }
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png);
  }
  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE || color_type == PNG_COLOR_TYPE_RGB) {
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  }
  if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png);
  }

  rgba = new uint8_t[height * width * 4];
  png_bytep* row_pointers = new png_bytep[height];
  for(uint32_t y = 0; y < height; y++)
    row_pointers[y] = reinterpret_cast<png_bytep>(rgba + y * width * 4);

  png_read_image(png, row_pointers);

  delete[] row_pointers;
  fclose(fp);
}

void write_uint32_be(FILE* fp, uint32_t v)
{
  v = htonl(v);
  fwrite(&v, sizeof(v), 1, fp);
}

void write_rgba_file(char* filename, uint32_t width, uint32_t height, uint8_t* rgba)
{
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    throw runtime_error{"Failed to open file for writing: " + string{filename}};
  }

  write_uint32_be(fp, 0x52474241u);
  write_uint32_be(fp, width);
  write_uint32_be(fp, height);
  fwrite(rgba, 4, width*height, fp);
  fclose(fp);
}

int main(int argc, char *argv[])
{
  try {
    if (argc != 3) {
      throw runtime_error{"Usage: png2rgba inImage.png outImage.rgba"};
    }

    uint32_t width;
    uint32_t height;
    uint8_t* rgba;
    read_png_file(argv[1], width, height, rgba);
    write_rgba_file(argv[2], width, height, rgba);

    delete[] rgba;
  } catch (const exception& e) {
    cerr << e.what() << "\n";
    return 1;
  }

  return 0;
}
