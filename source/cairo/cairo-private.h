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
typedef struct p2c_font_s p2c_font_t;

#define MAX_GSTATE 64 // Maximum nesting of graphics states
#define MAX_FORM_DEPTH 16 // Maximum nesting of form XObjects

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
  p2c_font_t 	*font;
  double 	font_size;		      // Tfs
  double	char_spacing;		    // Tc
  double	word_spacing;		    // Tw
  double	horiz_scaling;		  // Th = Tz / 100
  double	text_rise;		      // Ts
  int 		text_rendering_mode;

  p2c_colorspace_t fill_colorspace;
  p2c_colorspace_t stroke_colorspace;
} graphics_state_t;


// CID width range from a CIDFont /W array
typedef struct p2c_cid_width_s
{
  unsigned	first, last;		// CID range
  double	width;			      // Width in glyph space (1/1000 em)
} p2c_cid_width_t;

// Internal Fonts objects
typedef struct p2c_font_s
{
  char		      *ref_font_name;		// PDF font reference Name e.g. "F1"
  pdfio_dict_t	*dict;			      // Font dictionary (identifies a loaded font)
  const char   	*font_name;    		// Original Font Name e.g. "BCDEEE+Calibri"
  const char	  *subtype;		      // Font Subtype e.g. "TrueType", "Type0"
  int		        flags;			      // FontDescriptor /Flags
  bool		      is_embedded;		  // Font program came from the PDF
  bool		      is_cid;			      // Type0 font: 2-byte codes, CID -> GID
  bool		      is_type3;		      // Glyphs are content streams (/CharProcs)

  uint8_t   	  *data;         		// Font Binary data (owned by cairo_face once created)
  size_t    	  data_size;     		// size of data
					
  int           first_char;       // starting CID/GID
  int           last_char;       	// ending CID/GID

  double        *widths;         	// Extract from the /Widths array
  size_t 	      num_widths;	      // Number of Extracted /Widths Array
  double 	      missing_width;
  double	      width_scale;		  // Glyph space -> text space (0.001, Type3 uses FontMatrix)

  int 	        encoding_table[256];	// Code -> Unicode
  const char	  *glyph_names[256];	  // Code -> glyph name from /Differences
  FT_UInt 	    code_to_gid[256];	    // Simple fonts: code -> glyph index
  double	      code_widths[256];	    // Simple fonts: code -> advance (glyph space)

  FT_UInt	      *cid_to_gid;		  // CIDToGIDMap stream (NULL = Identity)
  size_t	      num_cid_to_gid;
  p2c_cid_width_t *cid_widths;		// Sorted /W ranges
  size_t	      num_cid_widths;
  double	      default_width;    // /DW

  // Type 3 fonts
  cairo_matrix_t  font_matrix;	 // /FontMatrix: glyph space -> text space
  pdf_content_t	  **type3_procs; // Code -> glyph procedure (256 entries)
  pdfio_dict_t	  *resources;	   // Resources used by the glyph procedures

  FT_Face         ft_face;        // Active FreeType face object initialized from 'data'
  cairo_font_face_t *cairo_face; 	// The face created for Cairo
} p2c_font_t;

void p2c_font_destroy(p2c_font_t *font);
void device_clear_fonts(p2c_device_t *dev);


// The complete device structure definition
struct cairo_device_s
{
  cairo_surface_t 	*surface;
  cairo_t 	  	    *cr;
  graphics_state_t 	gstack[MAX_GSTATE];
  int 		  	      gstack_ptr;

  // font context
  pdfio_dict_t 		  *font_dict;
  p2c_font_t        **fonts;    // Array of extracted font structures
  size_t            num_fonts;

  // Glyph outlines collected by clipping text render modes (4-7) until ET
  cairo_path_t		  **text_clip_paths;
  size_t		        num_text_clip_paths;

  // Form XObjects being run (to stop a form from drawing itself) and whether
  // a Type 3 glyph procedure is running
  pdfio_obj_t		    *forms[MAX_FORM_DEPTH];
  int			          num_forms;
  bool			        in_type3_glyph;

  pdfio_dict_t 		*xobject_dict;
  pdfio_obj_t 		*page_obj;
};

p2c_device_t* device_create(pdfrip_page_t *page, int dpi);
void 	      device_destroy(p2c_device_t *dev);
void 	      device_save_to_png(p2c_device_t *dev, const char *filename);

#endif // CAIRO_INTERNAL_H
