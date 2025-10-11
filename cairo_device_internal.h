#ifndef CAIRO_DEVICE_INTERNAL_H
#define CAIRO_DEVICE_INTERNAL_H

#include "cairo_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global verbose flag (declared in main)
extern int g_verbose;

#define MAX_GSTATE 64 // Maximum nesting of graphics states

// Internal color space representation
typedef enum
{
  CS_DEVICE_GRAY,
  CS_DEVICE_RGB,
  CS_DEVICE_CMYK,
} p2c_colorspace_t;

// Our internal graphics state structure
typedef struct
{
  double fill_rgb[3];
  double stroke_rgb[3];
  double line_width;
  double fill_alpha;
  double stroke_alpha;
  cairo_matrix_t text_matrix;
  cairo_matrix_t text_line_matrix;
  double text_leading;
  double font_size;
  char font_name[128];
  p2c_colorspace_t fill_colorspace;
  p2c_colorspace_t stroke_colorspace;
} graphics_state_t;

// The complete device structure definition
struct cairo_device_s
{
  cairo_surface_t *surface;
  cairo_t *cr;
  graphics_state_t gstack[MAX_GSTATE];
  int gstack_ptr;
  pdfio_dict_t *font_dict;
  pdfio_dict_t *xobject_dict;
};

#endif // CAIRO_DEVICE_INTERNAL_H
