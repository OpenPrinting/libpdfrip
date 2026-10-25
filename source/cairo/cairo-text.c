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
{
  // Clipping render modes (4-7) intersect the clip with all glyph outlines
  // shown in this text object.
  if (dev->num_text_clip_paths == 0)
    return;

  cairo_new_path(dev->cr);
  for (size_t i = 0; i < dev->num_text_clip_paths; i ++)
  {
    cairo_append_path(dev->cr, dev->text_clip_paths[i]);
    cairo_path_destroy(dev->text_clip_paths[i]);
  }
  cairo_clip(dev->cr);

  free(dev->text_clip_paths);
  dev->text_clip_paths = NULL;
  dev->num_text_clip_paths = 0;
}

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
                p2c_font_t *font,
                double font_size)
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Fonts are looked up (and warned about) by pdf_find_font()
  gs->font = font;
  gs->font_size = font_size;

  if (g_verbose && font)
  {
    fprintf(stderr,
            "DEBUG: Selected font '%s' (%s).\n",
            font->ref_font_name ? font->ref_font_name : "?",
            font->font_name ? font->font_name : "?");
  }
}

void
device_set_char_spacing(p2c_device_t *dev,
                        double spacing)
{
  dev->gstack[dev->gstack_ptr].char_spacing = spacing;
}

void
device_set_word_spacing(p2c_device_t *dev,
                        double spacing)
{
  dev->gstack[dev->gstack_ptr].word_spacing = spacing;
}

void
device_set_horizontal_scaling(p2c_device_t *dev,
                              double percent)
{
  dev->gstack[dev->gstack_ptr].horiz_scaling = percent / 100.0;
}

void
device_set_text_rise(p2c_device_t *dev,
                     double rise)
{
  dev->gstack[dev->gstack_ptr].text_rise = rise;
}

//
// 'font_lookup_glyph()' - Map a character code to a glyph index and its
//                         advance width w0 (in text space, per unit font size).
//

static void
font_lookup_glyph(const p2c_font_t *font,
                  unsigned code,
                  unsigned long *gid,
                  double *w0)
{
  double width;

  if (font->is_cid)
  {
    // Identity-H/V: the 2-byte code is the CID
    unsigned cid = code;

    if (font->cid_to_gid)
      *gid = cid < font->num_cid_to_gid ? font->cid_to_gid[cid] : 0;
    else
      *gid = cid;

    width = font->default_width;

    // Binary search the sorted /W ranges
    size_t lo = 0, hi = font->num_cid_widths;
    while (lo < hi)
    {
      size_t mid = (lo + hi) / 2;
      const p2c_cid_width_t *r = &font->cid_widths[mid];

      if (cid < r->first)
        hi = mid;
      else if (cid > r->last)
        lo = mid + 1;
      else
      {
        width = r->width;
        break;
      }
    }
  }
  else
  {
    *gid = font->code_to_gid[code & 255];
    width = font->code_widths[code & 255];
  }

  *w0 = width * font->width_scale;
}

//
// 'draw_glyphs()' - Paint positioned glyphs according to the text render mode.
//                   Glyph positions are in text space (relative to Tm).
//

static void
draw_glyphs(p2c_device_t *dev,
            cairo_glyph_t *glyphs,
            int num_glyphs)
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_t *cr = dev->cr;
  int mode = gs->text_rendering_mode;
  bool fill = (mode == 0 || mode == 2 || mode == 4 || mode == 6);
  bool stroke = (mode == 1 || mode == 2 || mode == 5 || mode == 6);
  bool clip = (mode >= 4);
  cairo_matrix_t font_matrix, check;

  if (num_glyphs == 0 || mode == 3)
    return;

  // The font matrix scales the 1-em glyph space to Tfs x Th and flips Y
  // because cairo glyphs are y-down while PDF text space is y-up.
  cairo_matrix_init(&font_matrix,
                    gs->font_size * gs->horiz_scaling, 0,
                    0, -gs->font_size,
                    0, 0);

  // "0 Tf" or a degenerate Tm (used for hidden text) would put the cairo
  // context into a permanent error state and stop all further drawing.
  cairo_matrix_multiply(&check, &font_matrix, &gs->text_matrix);
  if (cairo_matrix_invert(&check) != CAIRO_STATUS_SUCCESS)
    return;

  cairo_save(cr);

  // User space becomes text space (y-up)
  cairo_transform(cr, &gs->text_matrix);
  cairo_set_font_face(cr, gs->font->cairo_face);
  cairo_set_font_matrix(cr, &font_matrix);

  if (fill && !stroke && !clip)
  {
    // Fast path: normal filled text
    cairo_set_source_rgba(cr, gs->fill_rgb[0], gs->fill_rgb[1],
                          gs->fill_rgb[2], gs->fill_alpha);
    cairo_show_glyphs(cr, glyphs, num_glyphs);
    cairo_restore(cr);
    return;
  }

  cairo_new_path(cr);
  cairo_glyph_path(cr, glyphs, num_glyphs);

  // The path survives cairo_restore(); stroking afterwards makes the line
  // width use user space like the rest of the page.
  cairo_restore(cr);

  if (fill)
  {
    cairo_set_source_rgba(cr, gs->fill_rgb[0], gs->fill_rgb[1],
                          gs->fill_rgb[2], gs->fill_alpha);
    cairo_fill_preserve(cr);
  }

  if (stroke)
  {
    cairo_set_source_rgba(cr, gs->stroke_rgb[0], gs->stroke_rgb[1],
                          gs->stroke_rgb[2], gs->stroke_alpha);
    cairo_stroke_preserve(cr);
  }

  if (clip)
  {
    cairo_path_t **paths = realloc(dev->text_clip_paths,
                                   (dev->num_text_clip_paths + 1) * sizeof(*paths));

    if (paths)
    {
      dev->text_clip_paths = paths;
      dev->text_clip_paths[dev->num_text_clip_paths ++] = cairo_copy_path(cr);
    }
  }

  cairo_new_path(cr);
}

//
// 'draw_type3_glyphs()' - Paint Type 3 glyphs by running their glyph
//                         procedures in glyph space (PDF 32000-1 9.6.5).
//
// Like MuPDF, any visible render mode paints the glyph as its procedure
// describes, modes 3 and 7 are invisible, and Type 3 glyphs never clip.
//

static void
draw_type3_glyphs(p2c_device_t *dev,
                  const cairo_glyph_t *glyphs,
                  int num_glyphs)
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  p2c_font_t *font = gs->font;
  cairo_matrix_t text_matrix = gs->text_matrix, glyph_matrix, check;
  int level = dev->gstack_ptr;
  int verbose = g_verbose;

  // A Type 3 glyph showing Type 3 text could recurse without bound
  if (gs->text_rendering_mode == 3 || gs->text_rendering_mode == 7 ||
      !font->type3_procs || dev->in_type3_glyph)
    return;

  // Glyph space -> text space: /FontMatrix, then the Tfs x Th scaling
  cairo_matrix_init(&glyph_matrix,
                    gs->font_size * gs->horiz_scaling, 0,
                    0, gs->font_size,
                    0, 0);
  cairo_matrix_multiply(&glyph_matrix, &font->font_matrix, &glyph_matrix);

  // "0 Tf" or a degenerate matrix would put cairo into an error state
  cairo_matrix_multiply(&check, &glyph_matrix, &text_matrix);
  if (cairo_matrix_invert(&check) != CAIRO_STATUS_SUCCESS)
    return;

  // Glyph procedures run once per glyph; keep -v output readable
  g_verbose = 0;
  dev->in_type3_glyph = true;

  for (int i = 0; i < num_glyphs; i ++)
  {
    const pdf_content_t *proc = font->type3_procs[glyphs[i].index & 255];

    if (!proc)
      continue;

    device_save_state(dev);
    if (dev->gstack_ptr == level)
      break;				// Graphics state stack is full

    // Glyph procedures inherit the graphics state, including the fill color
    cairo_transform(dev->cr, &text_matrix);
    cairo_translate(dev->cr, glyphs[i].x, glyphs[i].y);
    cairo_transform(dev->cr, &glyph_matrix);

    pdf_content_run(dev, proc, font->resources);

    // Also undoes any unbalanced q inside the glyph procedure
    while (dev->gstack_ptr > level)
      device_restore_state(dev);
  }

  dev->in_type3_glyph = false;
  g_verbose = verbose;
}

//
// 'device_show_text()' - Show a string (Tj): map each code to a glyph, place
//                        it using the PDF widths and advance Tm (PDF 9.4.4).
//

void 
device_show_text(p2c_device_t *dev, 
		 const unsigned char *str,
		 size_t len) 
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  p2c_font_t *font = gs->font;
  cairo_glyph_t stack_glyphs[256], *glyphs = stack_glyphs;
  size_t bytes_per_code, num_codes, num_glyphs = 0;
  double x = 0.0;

  if (!font || !str || len == 0)
    return;

  bytes_per_code = font->is_cid ? 2 : 1;
  num_codes = len / bytes_per_code;

  if (num_codes > sizeof(stack_glyphs) / sizeof(stack_glyphs[0]))
  {
    if ((glyphs = malloc(num_codes * sizeof(*glyphs))) == NULL)
      return;
  }

  for (size_t i = 0; i + bytes_per_code <= len; i += bytes_per_code)
  {
    unsigned code = bytes_per_code == 2 ? (unsigned)(str[i] << 8 | str[i + 1]) : str[i];
    unsigned long gid;
    double w0, tx;

    font_lookup_glyph(font, code, &gid, &w0);

    glyphs[num_glyphs].index = gid;
    glyphs[num_glyphs].x = x;
    glyphs[num_glyphs].y = gs->text_rise;
    num_glyphs ++;

    // tx = (w0 * Tfs + Tc + Tw) * Th; Tw only applies to single-byte code 32
    tx = w0 * gs->font_size + gs->char_spacing;
    if (bytes_per_code == 1 && code == 32)
      tx += gs->word_spacing;
    x += tx * gs->horiz_scaling;
  }

  if (font->is_type3)
    draw_type3_glyphs(dev, glyphs, (int)num_glyphs);
  else if (font->cairo_face)
    draw_glyphs(dev, glyphs, (int)num_glyphs);

  cairo_matrix_translate(&gs->text_matrix, x, 0);

  if (glyphs != stack_glyphs)
    free(glyphs);
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
      device_show_text(dev, operands[i].value.string, operands[i].length);
    } 
    else if (operands[i].type == OP_TYPE_NUMBER) 
    {
      double adj = -operands[i].value.number / 1000.0 * gs->font_size * gs->horiz_scaling;
      cairo_matrix_translate(&gs->text_matrix, adj, 0);
    }
  }
}

void 
device_set_text_rendering_mode(p2c_device_t *dev, 
			       int mode) 
{
  dev->gstack[dev->gstack_ptr].text_rendering_mode = mode;
}
