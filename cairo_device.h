#ifndef CAIRO_DEVICE_H
#define CAIRO_DEVICE_H

#include <cairo/cairo.h>
#include "pdfio.h"

typedef struct cairo_device_s p2c_device_t;

// --- Device Lifecycle ---
p2c_device_t *device_create(pdfio_rect_t mediabox, int dpi);
void device_destroy(p2c_device_t *dev);
void device_save_to_png(p2c_device_t *dev, const char *filename);

// --- Graphice State Management ---
void device_save_state(p2c_device_t *dev);
void device_restore_state(p2c_device_t *dev);
void device_set_line_width(p2c_device_t *dev, double width);
void device_set_fill_rgb(p2c_device_t *dev, double r, double g, double b);
void device_set_stroke_rgb(p2c_device_t *dev, double r, double g, double b);
void device_set_graphics_state(p2c_device_t *dev, pdfio_obj_t *resources, const char *name);
void device_set_fill_gray(p2c_device_t *dev, double g);
void device_set_stroke_gray(p2c_device_t *dev, double g);
void device_set_fill_cmyk(p2c_device_t *dev, double c, double m, double y, double k);
void device_set_stroke_cmyk(p2c_device_t *dev, double c, double m, double y, double k);

// --- Path Construction ---
void device_move_to(p2c_device_t *dev, double x, double y);
void device_line_to(p2c_device_t *dev, double x, double y);
void device_curve_to(p2c_device_t *dev, double x1, double y1, double x2, double y2, double x3, double y3);
void device_rectangle(p2c_device_t *dev, double x, double y, double w, double h);
void device_close_path(p2c_device_t *dev);

// --- Path Painting ---
void device_stroke(p2c_device_t *dev);
void device_fill(p2c_device_t *dev);
void device_fill_preserve(p2c_device_t *dev);
void device_fill_even_odd(p2c_device_t *dev);
void device_fill_preserve_even_odd(p2c_device_t *dev);

// --- Clipping Paths ---
void device_clip(p2c_device_t *dev);
void device_clip_even_odd(p2c_device_t *dev);


#endif 
