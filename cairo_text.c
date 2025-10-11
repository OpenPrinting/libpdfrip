#include "cairo_device_internal.h"

// --- Text State ---
void device_begin_text(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Begin Text Object\n");
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_init_identity(&gs->text_matrix);
  cairo_matrix_init_identity(&gs->text_line_matrix);
}

void device_end_text(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: End Text Object\n");
  // No-op for Cairo, but important for state tracking.
}

void device_set_text_leading(p2c_device_t *dev, double leading)
{
  if (g_verbose)
    printf("DEBUG: Set Text Leading to %f\n", leading);
  dev->gstack[dev->gstack_ptr].text_leading = leading;
}

void device_move_text_cursor(p2c_device_t *dev, double tx, double ty)
{
  if (g_verbose)
    printf("DEBUG: Move Text Cursor by (%f, %f)\n", tx, ty);

  cairo_matrix_t trans_matrix;
  cairo_matrix_init_translate(&trans_matrix, tx, ty);

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_multiply(&gs->text_line_matrix, &trans_matrix, &gs->text_line_matrix);

  memcpy(&gs->text_matrix, &gs->text_line_matrix, sizeof(cairo_matrix_t));
}

void device_next_line(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Move to Next Line\n");
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  device_move_text_cursor(dev, 0, -gs->text_leading);
}

void device_set_text_matrix(p2c_device_t *dev, double a, double b, double c, double d, double e, double f)
{
  if (g_verbose)
    printf("DEBUG: Set Text Matrix to [%f %f %f %f %f %f]\n", a, b, c, d, e, f);

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_init(&gs->text_matrix, a, b, c, d, e, f);
  memcpy(&gs->text_line_matrix, &gs->text_matrix, sizeof(cairo_matrix_t));
}

void device_set_font(p2c_device_t *dev, const char *font_name, double size)
{
  if (g_verbose)
    printf("DEBUG: Set Font to '%s' at size %f\n", font_name, size);

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  gs->font_size = size;
  strncpy(gs->font_name, font_name, sizeof(gs->font_name) - 1);
  gs->font_name[sizeof(gs->font_name) - 1] = '\0';

  // Note: Actual font selection happens in device_show_text.
  // This could be improved by loading fonts from the PDF resources.
}

void device_show_text(p2c_device_t *dev, const char *str)
{
  if (g_verbose)
    printf("DEBUG: Show Text: \"%s\"\n", str);

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);

  // Perform crude font substitution. A more robust implementation would
  // use the dev->font_dict to find and load the actual font.
  if (strstr(gs->font_name, "Times") != NULL)
    cairo_select_font_face(dev->cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  else if (strstr(gs->font_name, "Helvetica") != NULL)
    cairo_select_font_face(dev->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  else
    cairo_select_font_face(dev->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(dev->cr, gs->font_size);

  cairo_save(dev->cr);
  cairo_transform(dev->cr, &gs->text_matrix);
  cairo_move_to(dev->cr, 0, 0);
  cairo_show_text(dev->cr, str);
  cairo_restore(dev->cr);

  cairo_text_extents_t extents;
  cairo_text_extents(dev->cr, str, &extents);
  cairo_matrix_translate(&gs->text_matrix, extents.x_advance, 0);
}

void device_show_text_kerning(p2c_device_t *dev, operand_t *operands, int num_operands)
{
  if (g_verbose)
    printf("DEBUG: Show Text with Kerning (TJ)\n");

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Set the font and color once before processing the array
  cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);
  if (strstr(gs->font_name, "Times") != NULL)
    cairo_select_font_face(dev->cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  else if (strstr(gs->font_name, "Helvetica") != NULL)
    cairo_select_font_face(dev->cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  else
    cairo_select_font_face(dev->cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(dev->cr, gs->font_size);

  for (int i = 0; i < num_operands; i++)
  {
    if (operands[i].type == OP_TYPE_STRING)
    {
      // This is a string, so we show it and advance the cursor.
      const char *str = operands[i].value.string;
      if (g_verbose) printf("DEBUG: TJ drawing string: \"%s\"\n", str);

      cairo_save(dev->cr);
      cairo_transform(dev->cr, &gs->text_matrix);
      cairo_move_to(dev->cr, 0, 0);
      cairo_show_text(dev->cr, str);
      cairo_restore(dev->cr);

      cairo_text_extents_t extents;
      cairo_text_extents(dev->cr, str, &extents);
      cairo_matrix_translate(&gs->text_matrix, extents.x_advance, 0);
    }
    else if (operands[i].type == OP_TYPE_NUMBER)
    {
      // This is a number for kerning. The PDF specification says the
      // adjustment is measured in thousandths of a text space unit.
      // A positive number moves the cursor to the left (tighter).
      double adjustment = -operands[i].value.number / 1000.0 * gs->font_size;

      if (g_verbose)
        printf("DEBUG: Applying kerning adjustment: %f units\n", adjustment);

      // Apply this adjustment directly to the current text matrix.
      cairo_matrix_translate(&gs->text_matrix, adjustment, 0);
    }
  }
}
