//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cairo-private.h"
#include "../pdf/pdfops-private.h"

void 
device_begin_text(p2c_device_t *dev) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Begin Text\n");

  cairo_matrix_init_identity(&dev->gstack[dev->gstack_ptr].text_matrix);
  cairo_matrix_init_identity(&dev->gstack[dev->gstack_ptr].text_line_matrix);
}

void 
device_end_text(p2c_device_t *dev) 
{}

void 
device_set_text_leading(p2c_device_t *dev, 
		      	double leading) 
{
  dev->gstack[dev->gstack_ptr].text_leading = leading;
}

void 
device_move_text_cursor(p2c_device_t *dev, 
		   	double tx, double ty) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_t trans;
  cairo_matrix_init_translate(&trans, tx, ty);
  cairo_matrix_multiply(&gs->text_line_matrix, &trans, &gs->text_line_matrix);
  gs->text_matrix = gs->text_line_matrix;
}

void 
device_next_line(p2c_device_t *dev) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  device_move_text_cursor(dev, 0, -gs->text_leading);
}

void 
device_set_text_matrix(p2c_device_t *dev, 
		       double a, double b, 
		       double c, double d, 
		       double e, double f) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_init(&gs->text_matrix, a, b, c, d, e, f);
  gs->text_line_matrix = gs->text_matrix;
}

void 
device_set_font(p2c_device_t *dev, 
		const char *font_name, 
		double font_size) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Map PDF font names to standard Cairo families (using strings)
  const char *family = "Sans";
  cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
  cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL;

  // Simple heuristic to pick the font family
  if (strstr(font_name, "Times") || 
       strstr(font_name, "Serif") || 
        strstr(font_name, "Roman")) 
  {
    family = "Serif";
  } 
  else if (strstr(font_name, "Courier") || 
	    strstr(font_name, "Mono") || 
	     strstr(font_name, "Typewriter")) 
  {
    family = "Monospace";
  }

  if (strstr(font_name, "Bold")) 
    weight = CAIRO_FONT_WEIGHT_BOLD;
  if (strstr(font_name, "Italic") || strstr(font_name, "Oblique")) 
    slant = CAIRO_FONT_SLANT_ITALIC;

  // Select the font using strings
  cairo_select_font_face(dev->cr, family, slant, weight);
  cairo_set_font_size(dev->cr, font_size);
  gs->font_size = font_size;

  // We pass dev->page because pdf2text's load_encoding expects a page object to find resources.
  load_encoding(dev->page, font_name, gs->encoding);
}

static void 
utf8_encode(int unicode, 
	    char *out, 
	    int *len) 
{
  if (unicode < 0x80) 
  { 
    *len=1; 
    out[0]=unicode; 
  }
  else if (unicode < 0x800) 
  { 
    *len=2;
    out[0]=0xC0|(unicode>>6); 
    out[1]=0x80|(unicode&0x3F); 
  }
  else 
  { 
    *len=3; 
    out[0]=0xE0|(unicode>>12); 
    out[1]=0x80|((unicode>>6)&0x3F); 
    out[2]=0x80|(unicode&0x3F); 
  }
  out[*len] = 0;
}

void 
device_show_text(p2c_device_t *dev, 
		 const char *str) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  cairo_save(dev->cr);

  // Get Text Matrix and Flip Y for upright text
  cairo_matrix_t tm = gs->text_matrix;
  cairo_matrix_scale(&tm, 1.0, -1.0); // Flip Y because Cairo is Top-Down but we set CTM to Bottom-Up
  cairo_transform(dev->cr, &tm);

  // Decode String to UTF-8 using the encoding table
  char *utf8_str = malloc(strlen(str) * 4 + 1);
  char *p = utf8_str;
  for (int i = 0; str[i]; i++) 
  {
    int code = (unsigned char)str[i];
    int unicode = gs->encoding[code];
    int len;
    utf8_encode(unicode, p, &len);
    p += len;
  }
  *p = 0;

  // Draw
  cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);
  cairo_show_text(dev->cr, utf8_str);

  // Advance
  cairo_text_extents_t extents;
  cairo_text_extents(dev->cr, utf8_str, &extents);

  cairo_restore(dev->cr);

  // Advance internal matrix (unflipped)
  cairo_matrix_translate(&dev->gstack[dev->gstack_ptr].text_matrix, extents.x_advance, 0);

  free(utf8_str);
}

void 
device_show_text_kerning(p2c_device_t *dev, 
		    	 operand_t *operands, 
			 int num_operands) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  for (int i=0; i<num_operands; i++) 
  {
    if (operands[i].type == OP_TYPE_STRING) 
    {
      device_show_text(dev, operands[i].value.string);
    } 
    else if (operands[i].type == OP_TYPE_NUMBER) 
    {
      double adj = -operands[i].value.number / 1000.0 * gs->font_size;
      cairo_matrix_translate(&dev->gstack[dev->gstack_ptr].text_matrix, adj, 0);
    }
  }
}

void 
device_set_text_rendering_mode(p2c_device_t *dev, 
			       int mode) 
{
  dev->gstack[dev->gstack_ptr].text_rendering_mode = mode;
}
