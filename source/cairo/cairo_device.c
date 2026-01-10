#include "cairo_device_internal.h"

// --- Device LifeCycle Functions ---

//
// 'device_create()' - Initializes the Cairo rendering environment. 
// 		       Sets up the pixel surface, coordinate system, and 
// 		       initial graphics state
//

p2c_device_t*				  // O - pointer to initialized cairo structure
device_create(pdfio_rect_t mediabox, 	// I - physical dimensions(co-ords) of PDF page
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
  double width = (mediabox.x2 - mediabox.x1) * scale;
  double height = (mediabox.y2 - mediabox.y1) * scale;

  if (g_verbose)
    printf("DEBUG: Creating Cairo surface: %.2fx%.2f pixels (scale: %.2f)\n", width, height, scale);

  // Create the underlying Cairo image surface and context
  dev->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
  dev->cr = cairo_create(dev->surface);

  // Transform coordinates: Apply DPI scale and flip Y-axis to match PDF origin
  cairo_scale(dev->cr, scale, scale);
  cairo_translate(dev->cr, 0, mediabox.y2 - mediabox.y1); // Move to bottom-left
  cairo_scale(dev->cr, 1.0, -1.0); // Flip Y so (+Y) is "up" as per PDF spec
  
  // Initialize the FreeType library for font rendering
  if (FT_Init_FreeType(&dev->ft_library)) 
  {
    fprintf (stderr, "ERROR: Could not initialize FreeType library. \n");
    free(dev);
    return (NULL);
  }

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
  dev->font_cache = NULL;

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

    // Clean up the font cache: Iterate through the linked list
    font_cache_entry_t *current = dev->font_cache;
    font_cache_entry_t *next;
    while (current)
    {
      next = current->next;
      cairo_font_face_destroy(current->cairo_face); // Release the cached font face
      free(current);                               // Free the cache entry struct
      current = next;
    }

    // Shut down the FreeType library instance if it was initialized
    if (dev->ft_library)
      FT_Done_FreeType(dev->ft_library);

    // Destroy the Cairo context and the image surface
    cairo_destroy(dev->cr);
    cairo_surface_destroy(dev->surface);

    // Final step: free the device structure itself
    free(dev);
  }
}

// 
// 'device_set_resources()' - Associates page-level resource dictionaries with the device
//

void 						  // O - Void
device_set_resources(p2c_device_t *dev, 	// I - active Rendering context
		     pdfio_dict_t *res_dict)	// I - resource dictionary of page
{
  if (g_verbose)
    printf("DEBUG: Setting page resources for the device.\n");

  // Retrieve the dictionary from the provided PDF resource object
  if (!res_dict)
  {
    // If no dictionary exists, clear internal pointers and return
    dev->font_dict = NULL;
    dev->xobject_dict = NULL;
    return;
  }

  // Locate the /Font dictionary within resources and store its reference
  pdfio_obj_t *font_res_obj = pdfioDictGetObj(res_dict, "Font");
  dev->font_dict = font_res_obj ? pdfioObjGetDict(font_res_obj) : NULL;
  if (g_verbose && dev->font_dict)
    printf("DEBUG: Found /Font resource dictionary.\n");

  // Locate the /XObject dictionary and store its reference
  pdfio_obj_t *xobject_res_obj = pdfioDictGetObj(res_dict, "XObject");
  dev->xobject_dict = xobject_res_obj ? pdfioObjGetDict(xobject_res_obj) : NULL;
  
  if (g_verbose && dev->xobject_dict)
    printf("DEBUG: Found /XObject resource dictionary.\n");
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

//
// 'device_set_page' - Sets the active PDF page object for the device.
//

void 					  // O - Void
device_set_page (p2c_device_t *dev, 	// I - Active Rendering Context
	 	 pdfio_obj_t *page)	// I - Page object currently being renderered
{
  // Store the pointer to the PDF page object in the device structure
  dev->page = page;
}

