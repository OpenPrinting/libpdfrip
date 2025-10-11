#include "cairo_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use the global verbose flag defined in the main file
extern int g_verbose;

#define MAX_GSTATE 64 // Maximum nesting of graphics states

// Simple CMYK to RGB conversion
static void cmyk_to_rgb(double c, double m, double y, double k, double *r, double *g, double *b)
{
  *r = (1.0 - c) * (1.0 - k);
  *g = (1.0 - m) * (1.0 - k);
  *b = (1.0 - y) * (1.0 - k);
}

typedef enum
{
  CS_DEVICE_GRAY,
  CS_DEVICE_RGB,
  CS_DEVICE_CMYK,
} p2c_colorspace_t;

typedef struct
{
  double fill_rgb[3];   // Current fill color
  double stroke_rgb[3]; // Current stroke color
  double line_width;    // Current line width
  double fill_alpha;    // current fill alpha
  double stroke_alpha;  // current stroke alpha
  p2c_colorspace_t fill_colorspace; 
  p2c_colorspace_t stroke_colorspace;
} graphics_state_t;


struct cairo_device_s
{
  cairo_surface_t *surface;
  cairo_t *cr;
  graphics_state_t gstack[MAX_GSTATE];
  int gstack_ptr;
};

// --- Device LifeCycle Functions ---

p2c_device_t *device_create(pdfio_rect_t mediabox, int dpi)
{
  // Allocate memory for our device struct and zero it out
  p2c_device_t *dev = calloc(1, sizeof(p2c_device_t));
  if (!dev)
  {
    fprintf(stderr, "ERROR: Could not allocate memory for Cairo device.\n");
    return (NULL);
  }

  // Create the Cairo surface and rendering context
  double scale = dpi / 72.0;
  double width = (mediabox.x2 - mediabox.x1) * scale;
  double height = (mediabox.y2 - mediabox.y1) * scale;

  if (g_verbose)
    printf("DEBUG: Creating Cairo surface: %.2fx%.2f pixels (scale: %.2f)\n", width, height, scale);

  dev->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
  dev->cr = cairo_create(dev->surface);

  // Set initial scale and transform the coordinate system to match PDF
  // (origin at the bottom-left corner).
  cairo_scale(dev->cr, scale, scale);
  cairo_translate(dev->cr, 0, mediabox.y2 - mediabox.y1);
  cairo_scale(dev->cr, 1.0, -1.0);

  // Initialize the default graphics state on the stack
  dev->gstack[0].fill_rgb[0] = 0.0; // Black
  dev->gstack[0].fill_rgb[1] = 0.0;
  dev->gstack[0].fill_rgb[2] = 0.0;
  dev->gstack[0].stroke_rgb[0] = 0.0;
  dev->gstack[0].stroke_rgb[1] = 0.0;
  dev->gstack[0].stroke_rgb[2] = 0.0;
  dev->gstack[0].line_width = 1.0;
  dev->gstack[0].fill_alpha = 1.0;
  dev->gstack[0].stroke_alpha = 1.0;
  dev->gstack[0].fill_colorspace = CS_DEVICE_GRAY;
  dev->gstack[0].stroke_colorspace = CS_DEVICE_GRAY;
  dev->gstack_ptr = 0;

  // Start with a clean white background
  cairo_set_source_rgb(dev->cr, 1.0, 1.0, 1.0); // White
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

void device_save_to_png(p2c_device_t *dev, const char *filename)
{
  if (g_verbose)
    printf("DEBUG: Writing surface to PNG: %s\n", filename);
  if (cairo_surface_write_to_png(dev->surface, filename) != CAIRO_STATUS_SUCCESS)
  {
    fprintf(stderr, "ERROR: Unable to write PNG to '%s'. \n", filename);
  }
}

// --- Graphics State Management ---

void device_save_state(p2c_device_t *dev)
{
  if (dev->gstack_ptr < (MAX_GSTATE - 1))
  {
    cairo_save(dev->cr);
    // Copy the custom graphics state to the next level on our stack
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
  cmyk_to_rgb(c,m,y,k, &gs->stroke_rgb[0], &gs->stroke_rgb[1], &gs->stroke_rgb[2]);
  gs->stroke_colorspace = CS_DEVICE_CMYK;
}

void device_set_graphics_state(p2c_device_t *dev, pdfio_obj_t *resources, const char *name)
{
  if (g_verbose)
    printf("DEBUG: Applying graphics state dictionary: /%s\n", name);

  pdfio_dict_t *res_dict = pdfioObjGetDict(resources);
  if (!res_dict)
    return;

  pdfio_obj_t *extgstate_obj = pdfioDictGetObj(res_dict, "ExtGState");
  if (!extgstate_obj)
    return;

  pdfio_dict_t *extgstate_dict = pdfioObjGetDict(extgstate_obj);
  if (!extgstate_dict)
    return;

  pdfio_obj_t *gs_obj = pdfioDictGetObj(extgstate_dict, name);
  if (!gs_obj)
    return;

  pdfio_dict_t *gs_dict = pdfioObjGetDict(gs_obj);
  if (!gs_dict)
    return;

  // Check for supported graphics state keys
  pdfio_obj_t *lw_obj = pdfioDictGetObj(gs_dict, "LW");
  if (lw_obj)
  {
    device_set_line_width(dev, pdfioObjGetNumber(lw_obj));
  }
  
  // Check for fill alpha
  pdfio_obj_t *ca_obj = pdfioDictGetObj(gs_dict, "ca");
  if (ca_obj)
  {
    graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    gs->fill_alpha = pdfioObjGetNumber(ca_obj);
    if (g_verbose) printf("DEBUG: Set fill alpha to %f\n", gs->fill_alpha);
  }

  // Check for stroke alpha
  pdfio_obj_t *CA_obj = pdfioDictGetObj(gs_dict, "CA");
  if(CA_obj)
  {
    graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    gs->stroke_alpha = pdfioObjGetNumber(CA_obj);
    if (g_verbose) printf("DEBUG: Set stroke alpha to %f\n", gs->stroke_alpha);
  }

}

// --- Path Construction ---

void device_move_to(p2c_device_t *dev, double x, double y)
{
  if (g_verbose)
    printf("DEBUG: Path Move To (%f, %f)\n", x, y);
  cairo_move_to(dev->cr, x, y);
}

void device_line_to(p2c_device_t *dev, double x, double y)
{
  if (g_verbose)
    printf("DEBUG: Path Line To (%f, %f)\n", x, y);
  cairo_line_to(dev->cr, x, y);
}

void device_curve_to(p2c_device_t *dev, double x1, double y1, double x2, double y2, double x3, double y3)
{
  if (g_verbose)
    printf("DEBUG: Path Curve To (%f,%f %f,%f %f,%f)\n", x1, y1, x2, y2, x3, y3);
  cairo_curve_to(dev->cr, x1, y1, x2, y2, x3, y3);
}

void device_rectangle(p2c_device_t *dev, double x, double y, double w, double h)
{
  if (g_verbose)
    printf("DEBUG: Path Rectangle (%f,%f size %f x %f)\n", x, y, w, h);
  cairo_rectangle(dev->cr, x, y, w, h);
}

void device_close_path(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Path Close\n");
  cairo_close_path(dev->cr);
}

// --- Path Painting ---

void device_stroke(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Paint Stroke\n");
  // Set the source color from our state before stroking
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2], gs->stroke_alpha);
  cairo_stroke(dev->cr);
}

void device_fill(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Paint Fill\n");
  // Set the source color from our state before filling
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2], gs->fill_alpha);
  cairo_fill(dev->cr);
}

void device_fill_preserve(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Paint Fill Preserve\n");
  // Set the source color from our state before filling
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2], gs->fill_alpha);
  cairo_fill_preserve(dev->cr);
}

void device_fill_even_odd(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Paint Fill (Even/Odd Rule)\n");
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2], gs->fill_alpha);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_fill(dev->cr);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING); // Reset to default
}

void device_fill_preserve_even_odd(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Paint Fill Preserve (Even/Odd Rule)\n");
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2], gs->fill_alpha);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_fill_preserve(dev->cr);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING); // Reset to default
}

// --- Clipping Paths ---

void device_clip(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Set Clip Path\n");
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING);
  cairo_clip(dev->cr);
  cairo_new_path(dev->cr); // Clipping consumes the path, start a new one
}

void device_clip_even_odd(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Set Clip Path (Even/Odd Rule)\n");
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_clip(dev->cr);
  cairo_new_path(dev->cr); // Clipping consumes the path, start a new one
}
