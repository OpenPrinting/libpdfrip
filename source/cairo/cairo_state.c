#include "cairo_device_internal.h"

// Simple CMYK to RGB conversion (internal to this file)
static void cmyk_to_rgb(double c, double m, double y, double k, double *r, double *g, double *b)
{
  *r = (1.0 - c) * (1.0 - k);
  *g = (1.0 - m) * (1.0 - k);
  *b = (1.0 - y) * (1.0 - k);
}

// --- Graphics State Management ---

void device_save_state(p2c_device_t *dev)
{
  if (dev->gstack_ptr < (MAX_GSTATE - 1))
  {
    cairo_save(dev->cr);
    memcpy(&dev->gstack[dev->gstack_ptr + 1], &dev->gstack[dev->gstack_ptr], sizeof(graphics_state_t));
    dev->gstack_ptr++;
    if (g_verbose)
      printf("DEBUG: Graphics state saved. New stack level: %d\n", dev->gstack_ptr);
  }
  else
  {
    fprintf(stderr, "ERROR: Graphics state stack overflow.\n");
  }
}

void device_restore_state(p2c_device_t *dev)
{
  if (dev->gstack_ptr > 0)
  {
    cairo_restore(dev->cr);
    dev->gstack_ptr--;
    if (g_verbose)
      printf("DEBUG: Graphics state restored. New stack level: %d\n", dev->gstack_ptr);
  }
  else
  {
    fprintf(stderr, "ERROR: Graphics state stack underflow.\n");
  }
}

void device_set_line_width(p2c_device_t *dev, double width)
{
  if (g_verbose)
    printf("DEBUG: Setting line width to: %f\n", width);
  dev->gstack[dev->gstack_ptr].line_width = width;
  cairo_set_line_width(dev->cr, width);
}

void device_set_fill_rgb(p2c_device_t *dev, double r, double g, double b)
{
  if (g_verbose)
    printf("DEBUG: Setting fill color to RGB(%f, %f, %f)\n", r, g, b);
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  gs->fill_rgb[0] = r;
  gs->fill_rgb[1] = g;
  gs->fill_rgb[2] = b;
  gs->fill_colorspace = CS_DEVICE_RGB;
}

void device_set_stroke_rgb(p2c_device_t *dev, double r, double g, double b)
{
  if (g_verbose)
    printf("DEBUG: Setting stroke color to RGB(%f, %f, %f)\n", r, g, b);
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  gs->stroke_rgb[0] = r;
  gs->stroke_rgb[1] = g;
  gs->stroke_rgb[2] = b;
  gs->stroke_colorspace = CS_DEVICE_RGB;
}

void device_set_fill_gray(p2c_device_t *dev, double g)
{
  if (g_verbose)
    printf("DEBUG: Setting fill color to Gray(%f)\n", g);
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  gs->fill_rgb[0] = g;
  gs->fill_rgb[1] = g;
  gs->fill_rgb[2] = g;
  gs->fill_colorspace = CS_DEVICE_GRAY;
}

void device_set_stroke_gray(p2c_device_t *dev, double g)
{
  if (g_verbose)
    printf("DEBUG: Setting stroke color to Gray(%f)\n", g);
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  gs->stroke_rgb[0] = g;
  gs->stroke_rgb[1] = g;
  gs->stroke_rgb[2] = g;
  gs->stroke_colorspace = CS_DEVICE_GRAY;
}

void device_set_fill_cmyk(p2c_device_t *dev, double c, double m, double y, double k)
{
  if (g_verbose)
    printf("DEBUG: Setting fill color to CMYK(%f, %f, %f, %f)\n", c, m, y, k);
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cmyk_to_rgb(c, m, y, k, &gs->fill_rgb[0], &gs->fill_rgb[1], &gs->fill_rgb[2]);
  gs->fill_colorspace = CS_DEVICE_CMYK;
}

void device_set_stroke_cmyk(p2c_device_t *dev, double c, double m, double y, double k)
{
  if (g_verbose)
    printf("DEBUG: Setting stroke color to CMYK(%f, %f, %f, %f)\n", c, m, y, k);
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cmyk_to_rgb(c, m, y, k, &gs->stroke_rgb[0], &gs->stroke_rgb[1], &gs->stroke_rgb[2]);
  gs->stroke_colorspace = CS_DEVICE_CMYK;
}

void device_set_graphics_state(p2c_device_t *dev, pdfio_obj_t *resources, const char *name)
{
  if (g_verbose)
    printf("DEBUG: Applying graphics state dictionary: /%s\n", name);

  pdfio_dict_t *res_dict = pdfioObjGetDict(resources);
  if (!res_dict) return;

  pdfio_obj_t *extgstate_obj = pdfioDictGetObj(res_dict, "ExtGState");
  if (!extgstate_obj) return;

  pdfio_dict_t *extgstate_dict = pdfioObjGetDict(extgstate_obj);
  if (!extgstate_dict) return;

  pdfio_obj_t *gs_obj = pdfioDictGetObj(extgstate_dict, name);
  if (!gs_obj) return;

  pdfio_dict_t *gs_dict = pdfioObjGetDict(gs_obj);
  if (!gs_dict) return;

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  pdfio_obj_t *val_obj;

  if ((val_obj = pdfioDictGetObj(gs_dict, "LW")) != NULL)
    device_set_line_width(dev, pdfioObjGetNumber(val_obj));

  if ((val_obj = pdfioDictGetObj(gs_dict, "ca")) != NULL)
  {
    gs->fill_alpha = pdfioObjGetNumber(val_obj);
    if (g_verbose) printf("DEBUG: Set fill alpha to %f\n", gs->fill_alpha);
  }

  if ((val_obj = pdfioDictGetObj(gs_dict, "CA")) != NULL)
  {
    gs->stroke_alpha = pdfioObjGetNumber(val_obj);
    if (g_verbose) printf("DEBUG: Set stroke alpha to %f\n", gs->stroke_alpha);
  }
}
