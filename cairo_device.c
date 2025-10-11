#include "cairo_device_internal.h"

// --- Device LifeCycle Functions ---

p2c_device_t *device_create(pdfio_rect_t mediabox, int dpi)
{
  p2c_device_t *dev = calloc(1, sizeof(p2c_device_t));
  if (!dev)
  {
    fprintf(stderr, "ERROR: Could not allocate memory for Cairo device.\n");
    return (NULL);
  }

  double scale = dpi / 72.0;
  double width = (mediabox.x2 - mediabox.x1) * scale;
  double height = (mediabox.y2 - mediabox.y1) * scale;

  if (g_verbose)
    printf("DEBUG: Creating Cairo surface: %.2fx%.2f pixels (scale: %.2f)\n", width, height, scale);

  dev->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
  dev->cr = cairo_create(dev->surface);

  cairo_scale(dev->cr, scale, scale);
  cairo_translate(dev->cr, 0, mediabox.y2 - mediabox.y1);
  cairo_scale(dev->cr, 1.0, -1.0);

  // Initialize the default graphics state
  dev->gstack[0] = (graphics_state_t){
      .fill_rgb = {0.0, 0.0, 0.0},
      .stroke_rgb = {0.0, 0.0, 0.0},
      .line_width = 1.0,
      .fill_alpha = 1.0,
      .stroke_alpha = 1.0,
      .text_leading = 0.0,
      .font_size = 1.0,
      .fill_colorspace = CS_DEVICE_GRAY,
      .stroke_colorspace = CS_DEVICE_GRAY};
  cairo_matrix_init_identity(&dev->gstack[0].text_matrix);
  cairo_matrix_init_identity(&dev->gstack[0].text_line_matrix);
  dev->gstack_ptr = 0;

  // Start with a clean white background
  cairo_set_source_rgb(dev->cr, 1.0, 1.0, 1.0);
  cairo_paint(dev->cr);

  return (dev);
}

void device_destroy(p2c_device_t *dev)
{
  if (dev)
  {
    if (g_verbose)
      printf("DEBUG: Destroying Cairo device.\n");
    cairo_destroy(dev->cr);
    cairo_surface_destroy(dev->surface);
    free(dev);
  }
}

void device_set_resources(p2c_device_t *dev, pdfio_obj_t *resources)
{
  if (g_verbose)
    printf("DEBUG: Setting page resources for the device.\n");

  pdfio_dict_t *res_dict = pdfioObjGetDict(resources);
  if (!res_dict)
  {
    dev->font_dict = NULL;
    dev->xobject_dict = NULL;
    return;
  }

  pdfio_obj_t *font_res_obj = pdfioDictGetObj(res_dict, "Font");
  dev->font_dict = font_res_obj ? pdfioObjGetDict(font_res_obj) : NULL;
  if (g_verbose && dev->font_dict)
    printf("DEBUG: Found /Font resource dictionary.\n");

  pdfio_obj_t *xobject_res_obj = pdfioDictGetObj(res_dict, "XObject");
  dev->xobject_dict = xobject_res_obj ? pdfioObjGetDict(xobject_res_obj) : NULL;
  if (g_verbose && dev->xobject_dict)
    printf("DEBUG: Found /XObject resource dictionary.\n");
}

void device_save_to_png(p2c_device_t *dev, const char *filename)
{
  if (g_verbose)
    printf("DEBUG: Writing surface to PNG: %s\n", filename);
  if (cairo_surface_write_to_png(dev->surface, filename) != CAIRO_STATUS_SUCCESS)
  {
    fprintf(stderr, "ERROR: Unable to write PNG to '%s'.\n", filename);
  }
}
