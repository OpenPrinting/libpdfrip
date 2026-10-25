//
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Drawing decoded images (Do with an image XObject).
//

#include "cairo-private.h"
#include <math.h>

//
// 'image_pattern()' - Create a pattern that maps the unit square of the
//                     current user space onto an image, with the first row
//                     of the image at the top (PDF 32000-1 8.9.4).
//

static cairo_pattern_t *
image_pattern(cairo_t         *cr,
              cairo_surface_t *image,
              bool            interpolate)
{
  int			width = cairo_image_surface_get_width(image),
			height = cairo_image_surface_get_height(image);
  double		ux = 1.0, uy = 0.0, vx = 0.0, vy = 1.0;
  cairo_pattern_t	*pattern = cairo_pattern_create_for_surface(image);
  cairo_matrix_t	matrix;
  cairo_filter_t	filter = CAIRO_FILTER_GOOD;

  cairo_matrix_init(&matrix, width, 0, 0, -height, 0, height);
  cairo_pattern_set_matrix(pattern, &matrix);

  // Repeat the edge pixels so the image edges are not blended with nothing
  cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);

  // Like poppler: without /Interpolate, an image enlarged 4x or more keeps
  // sharp pixels (scans of text, QR codes)
  if (!interpolate)
  {
    cairo_user_to_device_distance(cr, &ux, &uy);
    cairo_user_to_device_distance(cr, &vx, &vy);

    if (hypot(ux, uy) >= 4.0 * width || hypot(vx, vy) >= 4.0 * height)
      filter = CAIRO_FILTER_NEAREST;
  }

  cairo_pattern_set_filter(pattern, filter);

  return pattern;
}

//
// 'device_draw_image()' - Paint an image into the unit square, optionally
//                         through an alpha mask (/SMask or /Mask).
//

void
device_draw_image(p2c_device_t    *dev,
                  cairo_surface_t *image,
                  cairo_surface_t *mask,
                  bool            interpolate)
{
  graphics_state_t	*gs = &dev->gstack[dev->gstack_ptr];
  cairo_t		*cr = dev->cr;
  cairo_pattern_t	*source = image_pattern(cr, image, interpolate);

  cairo_save(cr);

  cairo_rectangle(cr, 0, 0, 1, 1);
  cairo_clip(cr);
  cairo_set_source(cr, source);

  if (mask)
  {
    cairo_pattern_t *alpha = image_pattern(cr, mask, interpolate);

    // The constant alpha (ca) applies on top of the mask
    if (gs->fill_alpha < 1.0)
      cairo_push_group(cr);

    cairo_mask(cr, alpha);

    if (gs->fill_alpha < 1.0)
    {
      cairo_pop_group_to_source(cr);
      cairo_paint_with_alpha(cr, gs->fill_alpha);
    }

    cairo_pattern_destroy(alpha);
  }
  else
    cairo_paint_with_alpha(cr, gs->fill_alpha);

  cairo_restore(cr);
  cairo_pattern_destroy(source);
}

//
// 'device_fill_image_mask()' - Paint the fill color through a stencil mask
//                              (an image XObject with /ImageMask true).
//

void
device_fill_image_mask(p2c_device_t    *dev,
                       cairo_surface_t *mask,
                       bool            interpolate)
{
  graphics_state_t	*gs = &dev->gstack[dev->gstack_ptr];
  cairo_t		*cr = dev->cr;
  cairo_pattern_t	*alpha = image_pattern(cr, mask, interpolate);

  cairo_save(cr);

  cairo_rectangle(cr, 0, 0, 1, 1);
  cairo_clip(cr);
  cairo_set_source_rgba(cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2],
                        gs->fill_alpha);
  cairo_mask(cr, alpha);

  cairo_restore(cr);
  cairo_pattern_destroy(alpha);
}