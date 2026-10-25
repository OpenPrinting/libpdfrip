//
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Types used by the image XObject decoder (pdf-image.c).
//

#ifndef PDF_IMAGE_PRIVATE_H
#define PDF_IMAGE_PRIVATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

// cairo image surfaces are limited to 32767 pixels per side
#define MAX_IMAGE_SIZE 32767

// Color models that image samples can be converted from
typedef enum image_kind_e
{
  IMAGE_GRAY,
  IMAGE_RGB,
  IMAGE_CMYK,
  IMAGE_INDEXED
} image_kind_t;

typedef struct image_colorspace_s
{
  image_kind_t   kind;         // Color model
  int            ncomps;       // Components per pixel in the samples
  bool           subtractive;  // Separation: tint 1 is full ink (dark)
  image_kind_t   base;         // Indexed: color model of the palette
  int            base_ncomps;  // Indexed: components per palette entry
  int            hival;        // Indexed: highest palette index
  unsigned char  *lookup;      // Indexed: (hival + 1) * base_ncomps bytes
} image_colorspace_t;

// Decoded samples, each row padded to a whole byte
typedef struct image_data_s
{
  unsigned char  *data;          // Samples
  int            width, height;  // Size in pixels
  int            ncomps;         // Components per pixel
  int            bpc;            // Bits per component
  size_t         stride;         // Bytes per row
  bool           jpeg;           // Decoded from DCTDecode (8-bit, own components)
} image_data_t;

typedef struct jpeg_error_s
{
  struct jpeg_error_mgr  mgr;   // libjpeg error manager
  jmp_buf                jump;  // Where to go when decoding fails
} jpeg_error_t;

#endif // PDF_IMAGE_PRIVATE_H
