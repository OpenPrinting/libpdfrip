//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef CAIRO_INTERNAL_H
#define CAIRO_INTERNAL_H

#include "cairo-device-private.h"
#include <cairo/cairo-ft.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ft2build.h>
#include FT_FREETYPE_H

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
typedef struct graphics_state_s
{
  double 	fill_rgb[3];
  double 	stroke_rgb[3];
  double 	line_width;
  double 	fill_alpha;
  double 	stroke_alpha;
  cairo_matrix_t text_matrix;
  cairo_matrix_t text_line_matrix;
  double 	text_leading;
  
  // Text State
  double 	font_size;
  char 		font_name[128];
  int 		encoding[256];
  int 		text_rendering_mode;

  p2c_colorspace_t fill_colorspace;
  p2c_colorspace_t stroke_colorspace;
} graphics_state_t;

typedef struct p2c_font_s
{
  const char	*ref_font_name;		// PDF font reference Name e.g. "F1"
  const char   	*font_name;    		// Original Font Name e.g. "BCDEEE+Calibri"
  const char	*encoding;		// Encoding type.
  uint8_t   	*data;         		// Font Binary data
  size_t    	data_size;     		// size of data
					
  int           first_char;       	// starting CID/GID
  int           last_char;        	// ending CID/GID
  double        *widths;           	// Extract from the /Widths array
  size_t 	num_widths;		// Number of Extracted /Widths Array

  int           encoding_table[256];

  FT_Face       ft_face;            	// Active FreeType face object initialized from 'data'
  cairo_font_face_t *cairo_face; 	// The face created for Cairo
} p2c_font_t;

void p2c_font_destroy(p2c_font_t *font);
void device_clear_fonts(p2c_device_t *dev);


// The complete device structure definition
struct cairo_device_s
{
  cairo_surface_t 	*surface;
  cairo_t 	  	*cr;
  graphics_state_t 	gstack[MAX_GSTATE];
  int 			gstack_ptr;

  // font context
  pdfio_dict_t 		*font_dict;
  p2c_font_t        	**fonts;    // Array of extracted font structures
  size_t            	num_fonts;

  // TODO: For XOBJECTS
  pdfio_dict_t 		*xobject_dict;
  pdfio_obj_t 		*page_obj;
};

p2c_device_t* device_create(pdfrip_page_t *page, int dpi);
void 	      device_destroy(p2c_device_t *dev);
void 	      device_save_to_png(p2c_device_t *dev, const char *filename);

#endif // CAIRO_INTERNAL_H
