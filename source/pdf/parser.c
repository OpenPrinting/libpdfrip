//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "parser.h"
#include "cairo-private.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Use the global verbose flag defined in the main file
extern int g_verbose;


#define INITIAL_OPERAND_CAPACITY 16
#define PDF_TOKEN_SIZE 65536


static bool
parser_context_init(parser_context_t *ctx,
                    p2c_device_t *dev,
                    pdfio_dict_t *resources)
{
  memset(ctx, 0, sizeof(*ctx));

  ctx->device = dev;
  ctx->resources = resources;
  ctx->operand_capacity = INITIAL_OPERAND_CAPACITY;
  ctx->operands = malloc(ctx->operand_capacity * sizeof(*ctx->operands));

  return ctx->operands != NULL;
}


static void
parser_context_destroy(parser_context_t *ctx)
{
  free(ctx->operands);
  memset(ctx, 0, sizeof(*ctx));
}


static operand_t*
parser_push_operand(parser_context_t *ctx)
{
  operand_t *operand;
  operand_t *new_operands;
  size_t new_capacity;

  if (ctx->num_operands == ctx->operand_capacity)
  {
    if (ctx->operand_capacity > SIZE_MAX / 2 / sizeof(*ctx->operands))
      return NULL;

    new_capacity = ctx->operand_capacity * 2;
    new_operands = realloc(ctx->operands,
                           new_capacity * sizeof(*ctx->operands));
    if (!new_operands)
      return NULL;

    ctx->operands = new_operands;
    ctx->operand_capacity = new_capacity;
  }

  // Callers fill in the value; clearing the whole 1 KB union is not needed
  operand = &ctx->operands[ctx->num_operands++];
  operand->type = OP_TYPE_NONE;
  operand->length = 0;
  return operand;
}


static bool
parser_has_number_operands(const parser_context_t *ctx, size_t count)
{
  size_t i;

  if (ctx->num_operands != count)
    return false;

  for (i = 0; i < count; i ++)
  {
    if (ctx->operands[i].type != OP_TYPE_NUMBER)
      return false;
  }

  return true;
}

static bool
parser_parse_number(const char *token, double *number)
{
  char *end;

  if (!token || !number ||
      !((token[0] >= '0' && token[0] <= '9') ||
        token[0] == '+' || token[0] == '-' || token[0] == '.'))
    return false;

  *number = strtod(token, &end);
  return end != token && *end == '\0';
}

static int
parser_hex_value(int ch)
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  else if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  else if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  else
    return -1;
}

//
// 'parser_decode_string()' - Convert a PDFio string token to raw bytes.
//
// PDFio returns literal strings as "(" followed by the already-unescaped
// bytes (the closing parenthesis is NOT included), and hex strings as "<"
// followed by the hex digits (the closing '>' is NOT included).
//

static size_t
parser_decode_string(const char *token, unsigned char *out, size_t outsize)
{
  size_t len = 0;

  if (token[0] == '(')
  {
    len = strlen(token + 1);
    if (len > outsize)
      len = outsize;
    memcpy(out, token + 1, len);
  }
  else
  {
    int hi = -1;

    for (const char *p = token + 1; *p && len < outsize; p ++)
    {
      int v = parser_hex_value(*p);

      if (v < 0)
        continue;

      if (hi < 0)
        hi = v;
      else
      {
        out[len ++] = (unsigned char)((hi << 4) | v);
        hi = -1;
      }
    }

    // An odd number of digits means the final digit is followed by an implicit 0
    if (hi >= 0 && len < outsize)
      out[len ++] = (unsigned char)(hi << 4);
  }

  return len;
}

// --- Operator Handler Functions ---
// Each function handles the logic for a single PDF operator.

static void 
handle_q(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator q (Save State)\n");
  device_save_state(ctx->device);
}

static void 
handle_Q(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator Q (Restore State)\n");
  device_restore_state(ctx->device);
}

static void 
handle_BT(parser_context_t *ctx) 
{
  device_begin_text(ctx->device);
}

static void 
handle_ET(parser_context_t *ctx) 
{
  device_end_text(ctx->device);
}

static void 
handle_Td(parser_context_t *ctx) 
{
  if (ctx->num_operands == 2 && 
       ctx->operands[0].type == OP_TYPE_NUMBER && 
        ctx->operands[1].type == OP_TYPE_NUMBER) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator Td (Move Text) with args (%f, %f)\n", 
	      	       ctx->operands[0].value.number, 
		       ctx->operands[1].value.number);

    device_move_text_cursor(ctx->device, ctx->operands[0].value.number, 
		          	 	 ctx->operands[1].value.number);
  }
}

static void 
handle_TD(parser_context_t *ctx) 
{
  if (ctx->num_operands == 2 && 
       ctx->operands[0].type == OP_TYPE_NUMBER && 
        ctx->operands[1].type == OP_TYPE_NUMBER) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator TD (Move text and Set Leading) with args (%f,%f)\n", 			  
		       ctx->operands[0].value.number, 
		       ctx->operands[1].value.number);
    device_set_text_leading(ctx->device, -ctx->operands[1].value.number);
    device_move_text_cursor(ctx->device, ctx->operands[0].value.number, 
		    		 	 ctx->operands[1].value.number);
  }
}

static void 
handle_T_star(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator T* (Next Line)\n");
  device_next_line(ctx->device);
}

static void 
handle_Tm(parser_context_t *ctx) 
{
  if(parser_has_number_operands(ctx, 6))
  { 
    device_set_text_matrix(ctx->device, ctx->operands[0].value.number, 
		    	   		ctx->operands[1].value.number, 
					ctx->operands[2].value.number,
			       		ctx->operands[3].value.number, 
					ctx->operands[4].value.number, 
					ctx->operands[5].value.number);
  }
}

static void 
handle_Tf(parser_context_t *ctx) 
{
  if (ctx->num_operands == 2 && 
       ctx->operands[0].type == OP_TYPE_NAME && 
        ctx->operands[1].type == OP_TYPE_NUMBER)
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator Tf (Set Font) with name %s and size %f\n", 
		       ctx->operands[0].value.name, 
		       ctx->operands[1].value.number);

    // The name is looked up in the resources of the page, form or Type 3
    // font being run; +1 skips the leading '/'
    device_set_font(ctx->device,
                    pdf_find_font(ctx->device, ctx->resources,
                                  ctx->operands[0].value.name + 1),
                    ctx->operands[1].value.number);
  }
}

static void 
handle_Tj(parser_context_t *ctx) 
{
  if (ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_STRING) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator Tj (Show Text) with %zu byte string\n", 
	       ctx->operands[0].length);

    device_show_text(ctx->device, ctx->operands[0].value.string,
                     ctx->operands[0].length);
  }
}

static void
handle_quote(parser_context_t *ctx)
{
  // string ' == T* string Tj
  if (ctx->num_operands == 1 &&
       ctx->operands[0].type == OP_TYPE_STRING)
  {
    device_next_line(ctx->device);
    device_show_text(ctx->device, ctx->operands[0].value.string,
                     ctx->operands[0].length);
  }
}

static void
handle_dquote(parser_context_t *ctx)
{
  // aw ac string " == aw Tw ac Tc string '
  if (ctx->num_operands == 3 &&
       ctx->operands[0].type == OP_TYPE_NUMBER &&
        ctx->operands[1].type == OP_TYPE_NUMBER &&
         ctx->operands[2].type == OP_TYPE_STRING)
  {
    device_set_word_spacing(ctx->device, ctx->operands[0].value.number);
    device_set_char_spacing(ctx->device, ctx->operands[1].value.number);
    device_next_line(ctx->device);
    device_show_text(ctx->device, ctx->operands[2].value.string,
                     ctx->operands[2].length);
  }
}

static void
handle_Tc(parser_context_t *ctx)
{
  if (parser_has_number_operands(ctx, 1))
    device_set_char_spacing(ctx->device, ctx->operands[0].value.number);
}

static void
handle_Tw(parser_context_t *ctx)
{
  if (parser_has_number_operands(ctx, 1))
    device_set_word_spacing(ctx->device, ctx->operands[0].value.number);
}

static void
handle_Tz(parser_context_t *ctx)
{
  if (parser_has_number_operands(ctx, 1))
    device_set_horizontal_scaling(ctx->device, ctx->operands[0].value.number);
}

static void
handle_TL(parser_context_t *ctx)
{
  if (parser_has_number_operands(ctx, 1))
    device_set_text_leading(ctx->device, ctx->operands[0].value.number);
}

static void
handle_Ts(parser_context_t *ctx)
{
  if (parser_has_number_operands(ctx, 1))
    device_set_text_rise(ctx->device, ctx->operands[0].value.number);
}

static void 
handle_TJ(parser_context_t *ctx) 
{
  if (ctx->num_operands > 0) 
  {
    device_show_text_kerning(ctx->device, ctx->operands, (int)ctx->num_operands);
  }
}

static void 
handle_w(parser_context_t *ctx) 
{
  if (ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NUMBER) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator w (Set Line Width) with arg %f\n", 
		       ctx->operands[0].value.number);
    device_set_line_width(ctx->device, ctx->operands[0].value.number);
  }
}

static void 
handle_rg(parser_context_t *ctx) 
{
  if (!ctx->ignore_color && parser_has_number_operands(ctx, 3))
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator rg (Set Fill RGB) with args (%f, %f, %f)\n", 
	       	      ctx->operands[0].value.number, 
	       	      ctx->operands[1].value.number, 
	              ctx->operands[2].value.number);

    device_set_fill_rgb(ctx->device, ctx->operands[0].value.number, 
		    	     	     ctx->operands[1].value.number, 
			     	     ctx->operands[2].value.number);
  }
}

static void 
handle_RG(parser_context_t *ctx) 
{
  if (!ctx->ignore_color && parser_has_number_operands(ctx, 3))
  { 
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator RG (Set Stroke RGB) with args (%f, %f, %f)\n", 
		       ctx->operands[0].value.number, 
		       ctx->operands[1].value.number,
		       ctx->operands[2].value.number);

    device_set_stroke_rgb(ctx->device, ctx->operands[0].value.number, 
		    	       	       ctx->operands[1].value.number, 
			       	       ctx->operands[2].value.number);
  }
}

static void 
handle_g(parser_context_t *ctx) 
{
  if (!ctx->ignore_color && ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NUMBER) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator g (Set Fill Gray) with arg %f\n", 
	       	       ctx->operands[0].value.number);

    device_set_fill_gray(ctx->device, ctx->operands[0].value.number);
  }
}

static void 
handle_G(parser_context_t *ctx) 
{
  if (!ctx->ignore_color && ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NUMBER) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator G (Set Stroke Gray) with arg %f\n", 
	  	       ctx->operands[0].value.number);

    device_set_stroke_gray(ctx->device, ctx->operands[0].value.number);
  }
}

static void 
handle_m(parser_context_t *ctx) 
{
  if (parser_has_number_operands(ctx, 2))
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator m (Move To) with args (%f, %f)\n", 
		       ctx->operands[0].value.number, 
		       ctx->operands[1].value.number);

    device_move_to(ctx->device, ctx->operands[0].value.number, 
		             	ctx->operands[1].value.number);
  }
}

static void 
handle_l(parser_context_t *ctx) 
{
  if (parser_has_number_operands(ctx, 2))
  { 
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator l (Line To) with args (%f, %f)\n", 
	       	       ctx->operands[0].value.number, 
		       ctx->operands[1].value.number);
    
    device_line_to(ctx->device, ctx->operands[0].value.number, 
		        	ctx->operands[1].value.number);
  }
}

static void 
handle_c(parser_context_t *ctx) 
{
  if (parser_has_number_operands(ctx, 6))
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator c (Curve To) with args (%f,%f %f,%f %f,%f)\n", 
		       ctx->operands[0].value.number, ctx->operands[1].value.number, 
		       ctx->operands[2].value.number, ctx->operands[3].value.number, 
		       ctx->operands[4].value.number, ctx->operands[5].value.number);

    device_curve_to(ctx->device, ctx->operands[0].value.number, ctx->operands[1].value.number, 
		                 ctx->operands[2].value.number, ctx->operands[3].value.number, 
			 	 ctx->operands[4].value.number, ctx->operands[5].value.number);
  }
}

static void
handle_v(parser_context_t *ctx)
{
  // v: Append curved segment (x2, y2, x3, y3).
  // Current point is (x1, y1).
  if (parser_has_number_operands(ctx, 4))
  {
    double x1, y1;
    device_get_current_point(ctx->device, &x1, &y1); // Get current point for x1, y1

    double x2 = ctx->operands[0].value.number;
    double y2 = ctx->operands[1].value.number;
    double x3 = ctx->operands[2].value.number;
    double y3 = ctx->operands[3].value.number;

    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator v (Curve) %f %f %f %f\n", x2, y2, x3, y3);

    device_curve_to(ctx->device, x1, y1, x2, y2, x3, y3);
  }
}

static void
handle_y(parser_context_t *ctx)
{
  // y: Append curved segment (x1, y1, x3, y3).
  // Final point (x3, y3) is also (x2, y2).
  if (parser_has_number_operands(ctx, 4))
  {
    double x1 = ctx->operands[0].value.number;
    double y1 = ctx->operands[1].value.number;
    double x3 = ctx->operands[2].value.number;
    double y3 = ctx->operands[3].value.number;

    if (g_verbose)
      fprintf(stderr, "DEBUG: Operator y (Curve) %f %f %f %f\n", x1, y1, x3, y3);

    // Pass x3, y3 as both the second control point and the end point
    device_curve_to(ctx->device, x1, y1, x3, y3, x3, y3);
  }
}

static void 
handle_re(parser_context_t *ctx) 
{
  if (parser_has_number_operands(ctx, 4))
  { 
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator re (Rectangle) with args (%f, %f, %f, %f)\n", 
		       ctx->operands[0].value.number, ctx->operands[1].value.number, 
		       ctx->operands[2].value.number, ctx->operands[3].value.number);

    device_rectangle(ctx->device, ctx->operands[0].value.number, ctx->operands[1].value.number, 
		    	          ctx->operands[2].value.number, ctx->operands[3].value.number);
  }
}

static void 
handle_h(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator h (Close Path)\n");

  device_close_path(ctx->device);
}

static void 
handle_S(parser_context_t *ctx) 
{
  if (g_verbose) 
   fprintf(stderr, "DEBUG: Operator S (Stroke Path)\n");

  device_stroke(ctx->device);
}

static void 
handle_f(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator f (Fill Path)\n");
  device_fill(ctx->device);
}

static void 
handle_f_star(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator f* (Fill Path Even-Odd)\n");

  device_fill_even_odd(ctx->device);
}

static void 
handle_B(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator B (Fill and Stroke Path)\n");

  device_fill_preserve(ctx->device);
  device_stroke(ctx->device);
}

static void 
handle_B_star(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator B* (Fill and Stroke Path Even-Odd)\n");

  device_fill_preserve_even_odd(ctx->device);
  device_stroke(ctx->device);
}

static void 
handle_b(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator b (Close, Fill, and Stroke Path)\n");

  device_close_path(ctx->device);
  device_fill_preserve(ctx->device);
  device_stroke(ctx->device);
}

static void 
handle_b_star(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator b* (Close, Fill, and Stroke Path Even-Odd)\n");

  device_close_path(ctx->device);
  device_fill_preserve_even_odd(ctx->device);
  device_stroke(ctx->device);
}

static void 
handle_n(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator n (New Path / No-Op)\n");
  // This is a no-op for our device, as paths are implicitly started.
}

static void 
handle_W(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator W (Clip Path)\n");

  device_clip(ctx->device);
}

static void 
handle_W_star(parser_context_t *ctx) 
{
  if (g_verbose) 
    fprintf(stderr, "DEBUG: Operator W* (Clip Path Even-Odd)\n");

  device_clip_even_odd(ctx->device);
}

static void 
handle_gs(parser_context_t *ctx) 
{
  if (ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NAME) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator gs (Set Graphics State) with name %s\n", 
		       ctx->operands[0].value.name);

    device_set_graphics_state(ctx->device, ctx->resources, ctx->operands[0].value.name + 1); // +1 to skip leading '/'
  }
}

static void
handle_cm(parser_context_t *ctx)
{
  // 'cm' expects 6 numbers on the stack: a b c d e f cm
  if (parser_has_number_operands(ctx, 6))
  {
    device_transform(ctx->device, ctx->operands[0].value.number, ctx->operands[1].value.number,
        		          ctx->operands[2].value.number, ctx->operands[3].value.number,
			          ctx->operands[4].value.number, ctx->operands[5].value.number);
  }
}

static void 
handle_cs(parser_context_t *ctx) 
{
  if (ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NAME) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator cs (Set fill Color Space) with name %s\n", 
		       ctx->operands[0].value.name);
  }
}

static void 
handle_CS(parser_context_t *ctx) 
{
  if (ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NAME) 
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator CS (Set Stroke Color Space) with name %s \n", 
		       ctx->operands[0].value.name);
  }
}

static void 
handle_k(parser_context_t *ctx) 
{
  if (!ctx->ignore_color && parser_has_number_operands(ctx, 4))
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator k (Set Fill CMYK) with args (%f, %f, %f, %f)\n", 
	      	       ctx->operands[0].value.number, ctx->operands[1].value.number, 
		       ctx->operands[2].value.number, ctx->operands[3].value.number);
   
    device_set_fill_cmyk(ctx->device, ctx->operands[0].value.number, 
		    	       	   ctx->operands[1].value.number, 
			      	   ctx->operands[2].value.number, 
			      	   ctx->operands[3].value.number);
  }
}

static void 
handle_K(parser_context_t *ctx) 
{
  if (!ctx->ignore_color && parser_has_number_operands(ctx, 4))
  {
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator K (Set Stroke CMYK) with args (%f, %f, %f, %f)\n", 
	      	       ctx->operands[0].value.number, ctx->operands[1].value.number, 
		       ctx->operands[2].value.number, ctx->operands[3].value.number);

    device_set_stroke_cmyk(ctx->device, ctx->operands[0].value.number, 
		    			ctx->operands[1].value.number, 
					ctx->operands[2].value.number, 
					ctx->operands[3].value.number);
  }
}

static void 
handle_Tr(parser_context_t *ctx) 
{
  if (ctx->num_operands == 1 && 
       ctx->operands[0].type == OP_TYPE_NUMBER) 
  {
    int mode = (int)ctx->operands[0].value.number;
    if (g_verbose) 
      fprintf(stderr, "DEBUG: Operator Tr (Set Text Rendering Mode) with mode %d\n", mode);
    // This is a placeholder for the actual device function you'll write
    // For now, let's just imagine it exists
    // device_set_text_rendering_mode(dev, mode);
    // Since our state is in the internal header, we can set it directly
    // from here for simplicity, but a dedicated function is cleaner.
    // For now, let's assume we don't have that function yet. We'll add it to graphics_state_t.
    // This is a simplified approach, a dedicated function in cairo_device.c would be better.
    device_set_text_rendering_mode(ctx->device, mode);
  }
}

static void
handle_d0(parser_context_t *ctx)
{
  // Colored Type 3 glyph: its procedure sets its own colors
}

static void
handle_d1(parser_context_t *ctx)
{
  // Uncolored Type 3 glyph: it is painted with the current color, so color
  // operators in the glyph procedure are ignored (PDF 32000-1 9.6.5)
  ctx->ignore_color = true;
}

//
// 'handle_Do()' - Paint an XObject (Do): an image, or a form XObject that is
//                 run like nested page content (PDF 32000-1 8.9 and 8.10).
//

static void
handle_Do(parser_context_t *ctx)
{
  p2c_device_t	*dev = ctx->device;
  const char	*name;
  pdfio_obj_t	*xobj;
  pdfio_dict_t	*xdict, *resources;
  pdfio_array_t	*matrix, *bbox;
  const char	*subtype;
  pdf_content_t	content;
  int		level;

  if (ctx->num_operands != 1 || ctx->operands[0].type != OP_TYPE_NAME)
    return;

  name    = ctx->operands[0].value.name + 1;
  xobj    = pdfioDictGetObj(pdf_dict_get_dict(ctx->resources, "XObject"), name);
  xdict   = pdfioObjGetDict(xobj);
  subtype = pdfioDictGetName(xdict, "Subtype");

  if (subtype && !strcmp(subtype, "Image"))
  {
    pdf_draw_image(dev, xobj, ctx->resources);
    return;
  }

  if (!subtype || strcmp(subtype, "Form"))
  {
    if (g_verbose)
      fprintf(stderr, "DEBUG: Skipping %s XObject '%s'.\n",
              subtype ? subtype : "unknown", name);
    return;
  }

  for (int i = 0; i < dev->num_forms; i ++)
  {
    if (dev->forms[i] == xobj)
    {
      fprintf(stderr, "WARNING: Form XObject '%s' draws itself, skipping.\n", name);
      return;
    }
  }

  if (dev->num_forms >= MAX_FORM_DEPTH)
  {
    fprintf(stderr, "WARNING: Form XObject '%s' is nested too deeply, skipping.\n", name);
    return;
  }

  if (!pdf_content_load(&content, xobj))
    return;

  matrix    = pdf_dict_get_array(xdict, "Matrix");
  bbox      = pdf_dict_get_array(xdict, "BBox");
  resources = pdf_dict_get_dict(xdict, "Resources");
  level     = dev->gstack_ptr;

  device_save_state(dev);

  if (pdfioArrayGetSize(matrix) == 6)
    device_transform(dev, pdfioArrayGetNumber(matrix, 0), pdfioArrayGetNumber(matrix, 1),
                          pdfioArrayGetNumber(matrix, 2), pdfioArrayGetNumber(matrix, 3),
                          pdfioArrayGetNumber(matrix, 4), pdfioArrayGetNumber(matrix, 5));

  if (pdfioArrayGetSize(bbox) == 4)
  {
    double x0 = pdfioArrayGetNumber(bbox, 0), y0 = pdfioArrayGetNumber(bbox, 1),
           x1 = pdfioArrayGetNumber(bbox, 2), y1 = pdfioArrayGetNumber(bbox, 3);

    device_rectangle(dev, x0, y0, x1 - x0, y1 - y0);
    device_clip(dev);
  }

  dev->forms[dev->num_forms ++] = xobj;

  // Forms without /Resources use the resources they are drawn with (PDF 1.1)
  pdf_content_run(dev, &content, resources ? resources : ctx->resources);

  dev->num_forms --;

  // Also undoes any unbalanced q inside the form
  while (dev->gstack_ptr > level)
    device_restore_state(dev);

  pdf_content_free(&content);
}

// --- Dispatch Table and Logic ---

// type for our handler functions
typedef void (*pdf_operator_handler_t)(parser_context_t *ctx);

// structure for lookup table entries
typedef struct 
{
  const char *name;
  pdf_operator_handler_t handler;
} pdf_operator_t;

// lookup table for Operators
// IMPORTANT: This table MUST be sorted alphabetically by operator name for bsearch to work.
static const pdf_operator_t operator_table[] = 
{
  {"\"", 	handle_dquote},
  {"'", 	handle_quote},
  {"B", 	handle_B},
  {"B*", 	handle_B_star},
  {"BT", 	handle_BT},
  {"CS", 	handle_CS},
  {"Do", 	handle_Do},
  {"ET", 	handle_ET},
  {"G", 	handle_G},
  {"K", 	handle_K},
  {"Q", 	handle_Q},
  {"RG", 	handle_RG},
  {"S", 	handle_S},
  {"T*", 	handle_T_star},
  {"TD", 	handle_TD},
  {"TJ", 	handle_TJ},
  {"TL", 	handle_TL},
  {"Tc", 	handle_Tc},
  {"Td", 	handle_Td},
  {"Tf", 	handle_Tf},
  {"Tj", 	handle_Tj},
  {"Tm", 	handle_Tm},
  {"Tr", 	handle_Tr},
  {"Ts", 	handle_Ts},
  {"Tw", 	handle_Tw},
  {"Tz", 	handle_Tz},
  {"W", 	handle_W},
  {"W*", 	handle_W_star},
  {"b", 	handle_b},
  {"b*", 	handle_b_star},
  {"c", 	handle_c},
  {"cm", 	handle_cm},
  {"cs", 	handle_cs},
  {"d0", 	handle_d0},
  {"d1", 	handle_d1},
  {"f", 	handle_f},
  {"f*", 	handle_f_star},
  {"g", 	handle_g},
  {"gs", 	handle_gs},
  {"h", 	handle_h},
  {"k", 	handle_k},
  {"l", 	handle_l},
  {"m", 	handle_m},
  {"n", 	handle_n},
  {"q", 	handle_q},
  {"re", 	handle_re},
  {"rg", 	handle_rg},
  {"v", 	handle_v},
  {"w", 	handle_w},
  {"y", 	handle_y},
};

static const size_t operator_table_size = sizeof(operator_table) / sizeof(operator_table[0]);

// Create a comparison function for bsearch
static int 
compare_operators(const void *a, 
		  const void *b) 
{
  const char *token = (const char *)a;
  const pdf_operator_t *op = (const pdf_operator_t *)b;
  return strcmp(token, op->name);
}

//
// 'content_add_token()' - Append one token to in-memory content.
//

static bool
content_add_token(pdf_content_t *content,
                  const char    *token)
{
  size_t len = strlen(token) + 1;

  if (content->num_tokens == content->alloc_tokens)
  {
    size_t alloc = content->alloc_tokens ? content->alloc_tokens * 2 : 1024;
    size_t *starts = realloc(content->starts, alloc * sizeof(*starts));

    if (!starts)
      return false;

    content->starts = starts;
    content->alloc_tokens = alloc;
  }

  if (content->used + len > content->alloc)
  {
    size_t alloc = content->alloc ? content->alloc * 2 : 16384;
    char *data;

    while (alloc < content->used + len)
      alloc *= 2;

    if ((data = realloc(content->data, alloc)) == NULL)
      return false;

    content->data = data;
    content->alloc = alloc;
  }

  content->starts[content->num_tokens ++] = content->used;
  memcpy(content->data + content->used, token, len);
  content->used += len;

  return true;
}

//
// 'content_read_stream()' - Read every token of an open stream, then close it.
//

static bool
content_read_stream(pdf_content_t  *content,
                    pdfio_stream_t *st,
                    char           *token)
{
  bool ret = true;

  while (pdfioStreamGetToken(st, token, PDF_TOKEN_SIZE))
  {
    if (!content_add_token(content, token))
    {
      ret = false;
      break;
    }
  }

  pdfioStreamClose(st);

  return ret;
}

//
// 'pdf_content_load()' - Read the content stream of a form XObject or
//                        Type 3 glyph procedure into memory.
//

bool
pdf_content_load(pdf_content_t *content,
                 pdfio_obj_t   *obj)
{
  pdfio_stream_t *st;
  char *token;
  bool ret;

  memset(content, 0, sizeof(*content));

  if (!obj || (st = pdfioObjOpenStream(obj, true)) == NULL)
    return false;

  if ((token = malloc(PDF_TOKEN_SIZE)) == NULL)
  {
    pdfioStreamClose(st);
    return false;
  }

  if ((ret = content_read_stream(content, st, token)) == false)
    pdf_content_free(content);

  free(token);

  return ret;
}

//
// 'pdf_content_free()' - Free in-memory content.
//

void
pdf_content_free(pdf_content_t *content)
{
  free(content->data);
  free(content->starts);
  memset(content, 0, sizeof(*content));
}

//
// 'parser_run_token()' - Push an operand or execute an operator.
//
// Returns false only when the operand stack cannot grow.
//

static bool
parser_run_token(parser_context_t *ctx,
                 const char       *token)
{
  double number;

  if (parser_parse_number(token, &number))
  {
    operand_t *operand = parser_push_operand(ctx);

    if (!operand)
      return false;

    operand->type = OP_TYPE_NUMBER;
    operand->value.number = number;

    if (g_verbose)
      fprintf(stderr, "DEBUG: Pushed number: %f\n", number);
  }
  else if (token[0] == '/')
  {
    operand_t *operand = parser_push_operand(ctx);

    if (!operand)
      return false;

    operand->type = OP_TYPE_NAME;
    snprintf(operand->value.name, sizeof(operand->value.name), "%s", token);

    if (g_verbose)
      fprintf(stderr, "DEBUG: Pushed name: %s\n", operand->value.name);
  }
  else if (token[0] == '(' || (token[0] == '<' && token[1] != '<'))
  {
    operand_t *operand = parser_push_operand(ctx);

    if (!operand)
      return false;

    operand->type = OP_TYPE_STRING;
    operand->length = parser_decode_string(token, operand->value.string,
                                           sizeof(operand->value.string));

    if (g_verbose)
      fprintf(stderr, "DEBUG: Pushed %zu byte string\n", operand->length);
  }
  // Array delimiters are currently ignored. Their strings and numbers are
  // kept as consecutive operands for the existing TJ device interface.
  else if (token[0] != '[' && token[0] != ']')
  {
    const pdf_operator_t *pdf_operator =
        bsearch(token, operator_table, operator_table_size,
                sizeof(pdf_operator_t), compare_operators);

    if (pdf_operator)
      pdf_operator->handler(ctx);
    else if (g_verbose)
      fprintf(stderr, "DEBUG: Unhandled operator: %s\n", token);

    ctx->num_operands = 0;
  }

  return true;
}

//
// 'pdf_content_run()' - Run in-memory content with the given resources.
//
// Each run has its own operand stack, so a form or Type 3 glyph can be run
// while the operator that triggered it still holds its operands.
//

void
pdf_content_run(p2c_device_t        *dev,
                const pdf_content_t *content,
                pdfio_dict_t        *resources)
{
  parser_context_t ctx;

  if (!parser_context_init(&ctx, dev, resources))
  {
    fprintf(stderr, "ERROR: Unable to allocate the PDF parser context.\n");
    return;
  }

  for (size_t i = 0; i < content->num_tokens; i ++)
  {
    if (!parser_run_token(&ctx, content->data + content->starts[i]))
    {
      fprintf(stderr, "ERROR: Unable to grow the PDF operand stack.\n");
      break;
    }
  }

  parser_context_destroy(&ctx);
}

void 
process_content_stream(p2c_device_t *dev, 
		       pdfrip_page_t *page_data)
{
  pdf_content_t content;
  char *token;

  if (!dev || !page_data)
  {
    fprintf(stderr, "ERROR: Cannot parse a content stream without a device and page.\n");
    return;
  }

  /*
   * Read all page content before running any of it: PDFio streams share the
   * file position, so loading an object (font, form, Type 3 glyph) while a
   * content stream is open would corrupt the rest of that stream.
   */
  memset(&content, 0, sizeof(content));

  if ((token = malloc(PDF_TOKEN_SIZE)) == NULL)
  {
    fprintf(stderr, "ERROR: Unable to allocate the PDF token buffer.\n");
    return;
  }

  for (size_t i = 0; i < page_data->num_streams; i ++)
  {
    pdfio_stream_t *st = pdfioPageOpenStream(page_data->object, i, true);

    if (st && !content_read_stream(&content, st, token))
    {
      fprintf(stderr, "ERROR: Unable to read the page content into memory.\n");
      break;
    }
  }

  free(token);

  pdf_content_run(dev, &content, page_data->resources_dict);
  pdf_content_free(&content);
}
