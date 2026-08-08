//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cairo-private.h"

// --- Device LifeCycle Functions ---

//
// 'device_create()' - Initializes the Cairo rendering environment. 
// 		       Sets up the pixel surface, coordinate system, and 
// 		       initial graphics state
//

p2c_device_t*				  
device_create(pdfrip_page_t *page, 	
	      int dpi)			
{
  p2c_device_t *dev = calloc(1, sizeof(p2c_device_t));
  if (!dev)
  {
    fprintf(stderr, "ERROR: Could not allocate memory for Cairo device.\n");
    return (NULL);
  }

  // Calculate scale factor based on target DPI (PDF base is 72 DPI)
  double scale = dpi / 72.0;
  double width = (page->mediaBox.x2 - page->mediaBox.x1) * scale;
  double height = (page->mediaBox.y2 - page->mediaBox.y1) * scale;

  if (g_verbose)
    printf("DEBUG: Creating Cairo surface: %.2fx%.2f pixels (scale: %.2f)\n", width, height, scale);

  dev->num_fonts = 0;
  
  dev->page_obj = page->object; 

  dev->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
  if (cairo_surface_status(dev->surface) != CAIRO_STATUS_SUCCESS)
  {
    free(dev);
    return NULL;
  }

  dev->cr = cairo_create(dev->surface);
  cairo_scale(dev->cr, scale, scale);
  cairo_translate(dev->cr, 0, page->mediaBox.y2 - page->mediaBox.y1); 
  cairo_scale(dev->cr, 1.0, -1.0); 

  dev->gstack[0] = (graphics_state_t)
  {
    .fill_rgb = {0.0, 0.0, 0.0},
    .stroke_rgb = {0.0, 0.0, 0.0},
    .line_width = 1.0,
    .fill_alpha = 1.0,
    .stroke_alpha = 1.0,
    .text_leading = 0.0,
    .font_size = 1.0,
    .text_rendering_mode = 0,
    .fill_colorspace = CS_DEVICE_GRAY,
    .stroke_colorspace = CS_DEVICE_GRAY
  };

  cairo_matrix_init_identity(&dev->gstack[0].text_matrix);
  cairo_matrix_init_identity(&dev->gstack[0].text_line_matrix);
  dev->gstack_ptr = 0;

  for (int i=0; i<256; i++) 
    dev->gstack[0].encoding[i] = i;

  cairo_set_source_rgb(dev->cr, 1.0, 1.0, 1.0);
  cairo_paint(dev->cr);

  return (dev);
}

//
// 'p2c_font_destroy()' - destroy the Font Glyphs
//

void
p2c_font_destroy(p2c_font_t *font)
{
  if (!font)
    return;

  if (font->cairo_face)
  {
    cairo_font_face_destroy(font->cairo_face);
    font->cairo_face = NULL;
    font->ft_face = NULL;
  }
  else if (font->ft_face)
  {
    FT_Done_Face(font->ft_face);
    font->ft_face = NULL;
  }

  free(font->data);
  free(font->widths);
  free(font);
}

//
// 'device_clear_fonts()' - free the font structures
//

void
device_clear_fonts(p2c_device_t *dev)
{
  if (!dev || !dev->fonts)
    return;

  /*
   * cairo_face owns ft_face through Cairo user data.
   * Destroy it before freeing font->data because an
   * FT_Memory_Face depends on that data.
   */

  for (size_t i = 0; i < dev->num_fonts; i++)
    p2c_font_destroy(dev->fonts[i]);

  free(dev->fonts);

  dev->fonts = NULL;
  dev->num_fonts = 0;
}

//
// 'device_destroy()' - frees all allocated resources safely
//

void 					  
device_destroy(p2c_device_t *dev)	
{
  if (!dev)
    return;

  /*
   * Destroy Cairo context first because it may retain
   * a reference to the selected Cairo font face.
   */
  if (dev->cr)
  {
    cairo_destroy(dev->cr);
    dev->cr = NULL;
  }
  
  device_clear_fonts(dev);
  
  if (dev->surface)
  {
    cairo_surface_destroy(dev->surface);
    dev->surface = NULL;
  }

  free(dev);
}

//
// 'device_save_to_png()' - Saves the current rendered surface to a PNG file
//

void 						  // O - Void
device_save_to_png(p2c_device_t *dev, 		// I - Active Rendering context
		   const char *filename)	// I - File path where PNG will be saved
{
  if (g_verbose)
    printf("DEBUG: Writing surface to PNG: %s\n", filename);

  // Use Cairo's built-in utility to write the image surface to the filesystem
  if (cairo_surface_write_to_png(dev->surface, filename) != CAIRO_STATUS_SUCCESS)
  {
    // Report an error if the surface cannot be written
    fprintf(stderr, "ERROR: Unable to write PNG to '%s'.\n", filename);
  }
}


