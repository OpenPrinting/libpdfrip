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

p2c_device_t*				  // O - pointer to initialized cairo structure
device_create(pdfrip_page_t *page, 	// I - Data related to PDF page
	      int dpi)			// I - target resolution for output image(DPI)
{
  // Allocate memory for the device structure and zero it out
  p2c_device_t *dev = calloc(1, sizeof(p2c_device_t));
  if (!dev)
  {
    // Allocation failed
    fprintf(stderr, "ERROR: Could not allocate memory for Cairo device.\n");
    return (NULL);
  }

  // Calculate scale factor based on target DPI (PDF base is 72 DPI)
  double scale = dpi / 72.0;

  // Determine pixel dimensions from the PDF MediaBox and scale
  double width = (page->mediaBox.x2 - page->mediaBox.x1) * scale;
  double height = (page->mediaBox.y2 - page->mediaBox.y1) * scale;

  if (g_verbose)
    printf("DEBUG: Creating Cairo surface: %.2fx%.2f pixels (scale: %.2f)\n", width, height, scale);

  // set Null values to 
  dev->num_fonts = 0;

  // Create the underlying Cairo image surface and context
  dev->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
  if (cairo_surface_status(dev->surface) != CAIRO_STATUS_SUCCESS)
  {
    free(dev);
    return NULL;
  }

  dev->cr = cairo_create(dev->surface);

  // Scale Cairo so 1 user unit = 1 PDF point (handled by scaling matrix)
  cairo_scale(dev->cr, scale, scale);

  // Flip Y axis (PDF 0,0 is bottom-left, Cairo is top-left)
  cairo_translate(dev->cr, 0, page->mediaBox.y2 - page->mediaBox.y1); 
  cairo_scale(dev->cr, 1.0, -1.0); 


  // Define and set the default initial graphics state
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
    .stroke_colorspace = CS_DEVICE_GRAY};

  // Initialize text matrices to identity
  cairo_matrix_init_identity(&dev->gstack[0].text_matrix);
  cairo_matrix_init_identity(&dev->gstack[0].text_line_matrix);
  dev->gstack_ptr = 0;

  // Initialize default encoding (WinAnsi fallback) to identity
  for (int i=0; i<256; i++) 
    dev->gstack[0].encoding[i] = i;

  // Prepare the surface with a default white background
  cairo_set_source_rgb(dev->cr, 1.0, 1.0, 1.0);
  cairo_paint(dev->cr);

  return (dev);
}

//
// 'device_destroy()' - frees all allocated resources 
//

void 					  // O - Void 
device_destroy(p2c_device_t *dev)	// I - pointer to structure to be freed
{
  if (dev)
  {
    if (g_verbose)
      printf("DEBUG: Destroying Cairo device.\n");

    // Destroy the Cairo context and the image surface
    cairo_destroy(dev->cr);
    cairo_surface_destroy(dev->surface);

    // Thinking that each page might have different font thingy, so freeing it for now
    // But if error occur, will look into this.
    for (size_t i = 0; i < dev->num_fonts; i++) 
    {
      // Free the raw data and the Cairo face for the previous page
      free(dev->fonts[i]->data);
      cairo_font_face_destroy(dev->fonts[i]->cairo_face);
      free(dev->fonts[i]);
    }
    dev->num_fonts = 0;

    // Final step: free the device structure itself
    free(dev);
  }
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
