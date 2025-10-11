#include "cairo_device_internal.h"

// --- Internal Helper Functions ---

// Sets the current cairo source to the device's fill color/alpha
static void _apply_fill_color(p2c_device_t *dev)
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2], gs->fill_alpha);
}

// Sets the current cairo source to the device's stroke color/alpha
static void _apply_stroke_color(p2c_device_t *dev)
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_set_source_rgba(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2], gs->stroke_alpha);
}

// --- Path Construction ---

void device_move_to(p2c_device_t *dev, double x, double y)
{
  if (g_verbose) printf("DEBUG: Path Move To (%f, %f)\n", x, y);
  cairo_move_to(dev->cr, x, y);
}

void device_line_to(p2c_device_t *dev, double x, double y)
{
  if (g_verbose) printf("DEBUG: Path Line To (%f, %f)\n", x, y);
  cairo_line_to(dev->cr, x, y);
}

void device_curve_to(p2c_device_t *dev, double x1, double y1, double x2, double y2, double x3, double y3)
{
  if (g_verbose) printf("DEBUG: Path Curve To (%f,%f %f,%f %f,%f)\n", x1, y1, x2, y2, x3, y3);
  cairo_curve_to(dev->cr, x1, y1, x2, y2, x3, y3);
}

void device_rectangle(p2c_device_t *dev, double x, double y, double w, double h)
{
  if (g_verbose) printf("DEBUG: Path Rectangle (%f,%f size %f x %f)\n", x, y, w, h);
  cairo_rectangle(dev->cr, x, y, w, h);
}

void device_close_path(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Path Close\n");
  cairo_close_path(dev->cr);
}

// --- Path Painting ---

void device_stroke(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Paint Stroke\n");
  _apply_stroke_color(dev);
  cairo_stroke(dev->cr);
}

void device_fill(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Paint Fill\n");
  _apply_fill_color(dev);
  cairo_fill(dev->cr);
}

void device_fill_preserve(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Paint Fill Preserve\n");
  _apply_fill_color(dev);
  cairo_fill_preserve(dev->cr);
}

void device_fill_even_odd(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Paint Fill (Even/Odd Rule)\n");
  _apply_fill_color(dev);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_fill(dev->cr);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING); // Reset to default
}

void device_fill_preserve_even_odd(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Paint Fill Preserve (Even/Odd Rule)\n");
  _apply_fill_color(dev);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_fill_preserve(dev->cr);
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING); // Reset to default
}

// --- Clipping Paths ---

void device_clip(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Set Clip Path\n");
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING);
  cairo_clip(dev->cr);
  cairo_new_path(dev->cr);
}

void device_clip_even_odd(p2c_device_t *dev)
{
  if (g_verbose) printf("DEBUG: Set Clip Path (Even/Odd Rule)\n");
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_clip(dev->cr);
  cairo_new_path(dev->cr);
}
