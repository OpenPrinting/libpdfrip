#include "pdfio.h"
#include <cairo/cairo.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


// -- Operator Analysis Structure --
typedef struct 
{
  char name[64]; // Operator name
  int count;     // Number of times it appears
} operator_count_t;

// -- Global variables for analysis  ---
static operator_count_t *op_counts = NULL;
static int num_op_counts = 0;


// -- Graphics State Structure --
typedef struct
{
  // ToDo -- Add more state variable here later (fonts, etc)
  double fill_rgb[3]; // Fill color (red, green, blue)
  double stroke_rgb[3]; // Stroke color (red, green, blue)
  double line_width;  // Line width in points
} graphics_state_t;

// --- Global variables for rendering ---
#define MAX_GSTATE 64 // Maximum nesting of q/Q operators
static graphics_state_t gstack[MAX_GSTATE];
static int  gstack_ptr = 0; // Current position on the stack
#define MAX_OPERANDS 256 // Maximum number of operands for a command
static double operands[MAX_OPERANDS]; // the operand stack
static int num_operands = 0; // Number of operands on the stack
static int g_verbose = 0;

// 
// 'push_gstate()' -- Save teh current graphics state.
//
static void
push_gstate(cairo_t *cr)
{
  if (gstack_ptr < (MAX_GSTATE -1 ))
  {
    cairo_save(cr);
    memcpy(&gstack[gstack_ptr + 1], &gstack[gstack_ptr], sizeof(graphics_state_t));
    gstack_ptr++;

    if (g_verbose)
      printf("DEBUG: Saved graphics state (level %d).\n", gstack_ptr);
  }
  else
  {
    fprintf(stderr, "ERROR: Graphics state stack overflow!\n");
  }
}

//
// 'pop_gstate()' - Restore the previous graphics state.
//
static void
pop_gstate(cairo_t *cr)
{
  if (gstack_ptr > 0)
  {
    cairo_restore(cr);
    gstack_ptr--;

    if (g_verbose)
      printf("DEBUG: Restored graphics state (level %d). \n", gstack_ptr);
  }
  else
  {
    fprintf(stderr, "ERROR: Graphics state stack underflow!\n");
  }
}

//
// 'increment_op_count()' - Find and increment the count for an operator.
//
static void
increment_op_count(const char *token)
{
  // First, search if we already have this operator
  for (int i = 0; i < num_op_counts; i++)
  {
    if (!strcmp(op_counts[i].name, token))
    {
      op_counts[i].count++;
      return;
    }
  }

  // If not found, add a new entry
  void *temp = realloc(op_counts, (num_op_counts + 1) * sizeof(operator_count_t));
  if (!temp)
  {
    fprintf(stderr, "ERROR: Out of memory for operator counts!\n");
    return;
  }
  op_counts = temp;

  strncpy(op_counts[num_op_counts].name, token, sizeof(op_counts[0].name) - 1);
  op_counts[num_op_counts].name[sizeof(op_counts[0].name) - 1] = '\0';
  op_counts[num_op_counts].count = 1;
  num_op_counts++;
}


//
// 'analyze_content_stream()' -- Count all operators in the content stream.
//
static void
analyze_content_stream(pdfio_stream_t *st)
{
  char token[1024];

  while (pdfioStreamGetToken(st, token, sizeof(token)))
  {
    // We only care about operators, which are not numbers.
    if (!isdigit(token[0]) && token[0] != '.' && token[0] != '-')
    {
      increment_op_count(token);
    }
  }
}





//
// 'process_content_stream()' - Process the content stream for a page
//
static void
process_content_stream(cairo_t *cr, pdfio_stream_t *st,pdfio_obj_t *resources)
{
  char token[1024]; // Token from stream

  puts("\n---- Executing Page 1 Content Stream ---");
  while (pdfioStreamGetToken(st, token, sizeof(token)))
  {
    if (isdigit(token[0]) || token[0] == '.' || token[0] == '-')
    {
      // We have a number, push it on the operand stack
      if (num_operands < MAX_OPERANDS)
        operands[num_operands++] = atof(token);
    }
    else
    {
      // We have a command, process it...
      if (g_verbose)
        printf("DEBUG: Executing command '%s' with '%d' operands. \n", token, num_operands);
      if (!strcmp(token, "q"))
      {
        // Save graphics State
        push_gstate(cr);
      }
      else if (!strcmp(token, "Q"))
      {
        // Restore graphics state
        pop_gstate(cr);
      }
      else if (!strcmp(token, "cm"))
      {
        // Concatenate matrix
        if (num_operands == 6)
        {
          cairo_matrix_t matrix;
          cairo_matrix_init(&matrix, operands[0], operands[1], operands[2],operands[3], operands[4], operands[5]);
          cairo_transform(cr, &matrix);

          if (g_verbose)
            printf("DEBUG: Concatenate matrix [%g %g %g %g %g %g].\n", operands[0], operands[1], operands[2], operands[3], operands[4], operands[5]);
        }
      }
      else if (!strcmp(token, "w"))
      {
        // Set line width
        if (num_operands == 1)
        {
          gstack[gstack_ptr].line_width = operands[0];
          cairo_set_line_width(cr, operands[0]);

          if (g_verbose)
            printf("DEBUG: Set line width to %g. \n",operands[0]);
        }
      }
      else if (!strcmp(token, "rg"))
      {
        // Set fill Color (RGB)
        if (num_operands == 3)
        {
          gstack[gstack_ptr].fill_rgb[0] = operands[0];
          gstack[gstack_ptr].fill_rgb[1] = operands[1];
          gstack[gstack_ptr].fill_rgb[2] = operands[2];
          //cairo_set_source_rgb(cr, operands[0], operands[1], operands[2]);

          if (g_verbose)
            printf("DEBUG: Set fill color to %g %g %g \n", operands[0], operands[1], operands[2]);
        }
      }
      else if (!strcmp(token, "RG"))
      {
        // Set Stroke Color (RGB)
        if (num_operands == 3)
        {
          gstack[gstack_ptr].stroke_rgb[0] = operands[0];
          gstack[gstack_ptr].stroke_rgb[1] = operands[1];
          gstack[gstack_ptr].stroke_rgb[2] = operands[2];
          //cairo_set_source_rgb(cr, operands[0], operands[1], operands[2]);

          if (g_verbose)
            printf("DEBUG: Set stroke color to %g %g %g \n", operands[0], operands[1], operands[2]);

        }
      }
      else if (!strcmp(token, "g"))
      {
        // Set fill Color (GrayScale)
        if (num_operands == 1)
        {
          gstack[gstack_ptr].fill_rgb[0] = operands[0];
          gstack[gstack_ptr].fill_rgb[1] = operands[0];
          gstack[gstack_ptr].fill_rgb[2] = operands[0];
          //cairo_set_source_rgb(cr, operands[0], operands[0], operands[0]);

          if (g_verbose)
            printf("DEBUG: Set fill color to gray %g\n", operands[0]);
        }
      }
      else if (!strcmp(token, "G"))
      {
        // Set Stroke Color (GrayScale)
        if (num_operands == 1)
        {
          gstack[gstack_ptr].stroke_rgb[0] = operands[0];
          gstack[gstack_ptr].stroke_rgb[1] = operands[0];
          gstack[gstack_ptr].stroke_rgb[2] = operands[0];

          // To set a gray color in Cairo, you set R, G, and B to the same value.
          //cairo_set_source_rgb(cr, operands[0], operands[0], operands[0]);

          if (g_verbose)
            printf("DEBUG: Set stroke color to gray %g \n", operands[0]);
        }
      }
      else if (!strcmp(token, "m"))
      {
        // Move to 
        if (num_operands == 2)
        {
          cairo_move_to(cr, operands[0], operands[1]);

          if (g_verbose)
            printf("DEBUG: Move to (%g, %g). \n", operands[0], operands[1]);
        }
      }
      else if (!strcmp(token, "l"))
      {
        // Line to
        if (num_operands == 2)
        {
          cairo_line_to(cr, operands[0], operands[1]);

          if (g_verbose)
              printf("DEBUG: Line to (%g %g).\n", operands[0], operands[1]);
        }
      }
      else if (!strcmp(token, "c"))
      {
        // Curve to
        if (num_operands == 6)
        {
          cairo_curve_to(cr, operands[0], operands[1], operands[2], operands[3],operands[4], operands[5]);

          if (g_verbose)
            printf("DEBUG: Curve to (%g, %g) with control points (%g, %g) and (%g, %g).\n", operands[4], operands[5], operands[0], operands[1], operands[2], operands[3]);
      
        }
      }
      else if (!strcmp(token,"v"))
      {
        // Curve to (initial point replicated)
        if (num_operands == 4)
        {
          double curr_x, curr_y;
          cairo_get_current_point (cr, &curr_x, &curr_y);
          cairo_curve_to(cr, curr_x, curr_y, operands[0], operands[1], operands[2], operands[3]);

          if (g_verbose)
            printf("DEBUG: Curve 'v' to (%g, %g) with control points (%g, %g) and (%g, %g).\n", operands[2], operands[3], curr_x, curr_y, operands[0], operands[1]);
        }
      }
      else if (!strcmp(token, "y"))
      {
        // Curve to (final point replicated)
        if (num_operands == 4)
        {
          double curr_x, curr_y;
          cairo_get_current_point(cr, &curr_x, &curr_y);
          cairo_curve_to(cr, operands[0], operands[1], operands[2], operands[3], operands[2], operands[3]);

          if (g_verbose)
            printf("DEBUG: Curve 'y' to (%g, %g) with control points (%g, %g) and (%g, %g). \n", operands[2], operands[2], operands[0], operands[1], operands[2], operands[3]);
        }
      }
      else if (!strcmp(token, "re"))
      {
        // Rectangle
        if (num_operands == 4)
        {
          cairo_rectangle(cr, operands[0], operands[1], operands[2], operands[3]);

          if (g_verbose)
            printf("DEBUG: Rectangle at (%g, %g) size (%g %g) \n", operands[0],operands[1], operands[2], operands[3]);
        }
      }
      else if (!strcmp(token, "h"))
      {
        // Close Path
        cairo_close_path(cr);

        if (g_verbose)
          printf("DEBUG: Close path.\n");
      }
      else if (!strcmp(token, "S"))
      {
        // Stroke Path
        cairo_set_source_rgb(cr, gstack[gstack_ptr].stroke_rgb[0], gstack[gstack_ptr].stroke_rgb[1], gstack[gstack_ptr].stroke_rgb[2]);
        cairo_stroke(cr);

        if (g_verbose)
          printf("DEBUG: Stroke Path.\n");
      }
      else if (!strcmp(token, "f"))
      {
        // Fill Path
        cairo_set_source_rgb(cr, gstack[gstack_ptr].fill_rgb[0], gstack[gstack_ptr].fill_rgb[1], gstack[gstack_ptr].fill_rgb[2]);
        cairo_fill(cr);

        if (g_verbose)
          printf("DEBUG: Fill Path.\n");
      }
      else if (!strcmp(token, "B"))
      {
        // Fill and then stroke the path
        cairo_set_source_rgb(cr, gstack[gstack_ptr].fill_rgb[0],gstack[gstack_ptr].fill_rgb[1], gstack[gstack_ptr].fill_rgb[2]);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, gstack[gstack_ptr].stroke_rgb[0],gstack[gstack_ptr].stroke_rgb[1],gstack[gstack_ptr].stroke_rgb[2]);
        cairo_stroke(cr);

        if (g_verbose)
          printf("DEBUG: Fill and stroke path ('B').\n");
      }
      else if (!strcmp(token, "b"))
      {
        // Close, fill, and then stroke the path
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, gstack[gstack_ptr].fill_rgb[0], gstack[gstack_ptr].fill_rgb[1], gstack[gstack_ptr].fill_rgb[2]);
        cairo_fill_preserve(cr);
        cairo_set_source_rgb(cr, gstack[gstack_ptr].stroke_rgb[0],gstack[gstack_ptr].stroke_rgb[1], gstack[gstack_ptr].stroke_rgb[2]);
        cairo_stroke(cr);

        if (g_verbose)
          printf("DEBUG: Close, fill, and stroke path ('b').\n");
      }
      else if (!strcmp(token, "f*"))
      {
        // Fill Path (Even-Odd)
        cairo_set_source_rgb(cr, gstack[gstack_ptr].fill_rgb[0], gstack[gstack_ptr].fill_rgb[1], gstack[gstack_ptr].fill_rgb[2]);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_fill(cr);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);

        if (g_verbose)
          printf("DEBUG: Fill Path with even-odd rule ('f*').\n");
      }
      else if (!strcmp(token, "B*"))
      {
        // Fill and then stroke the path (Even-Odd)
        cairo_set_source_rgb(cr, gstack[gstack_ptr].fill_rgb[0], gstack[gstack_ptr].fill_rgb[1], gstack[gstack_ptr].fill_rgb[2]);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_fill_preserve(cr);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
        cairo_set_source_rgb(cr, gstack[gstack_ptr].stroke_rgb[0], gstack[gstack_ptr].stroke_rgb[1], gstack[gstack_ptr].stroke_rgb[2]);
        cairo_stroke(cr);

        if (g_verbose)
          printf("DEBUG: fill and stroke path with even-odd rule ('B*').\n");
      }
      else if (!strcmp(token, "b*"))
      {
        // Close, fill, and then stroke the path (Even-Odd)
        cairo_close_path(cr);
        cairo_set_source_rgb(cr, gstack[gstack_ptr].fill_rgb[0], gstack[gstack_ptr].fill_rgb[1], gstack[gstack_ptr].fill_rgb[2]);
            cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
            cairo_fill_preserve(cr);
            cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING); 
            cairo_set_source_rgb(cr, gstack[gstack_ptr].stroke_rgb[0], gstack[gstack_ptr].stroke_rgb[1], gstack[gstack_ptr].stroke_rgb[2]);
            cairo_stroke(cr);

            if (g_verbose)
              printf("DEBUG: Close, fill and stroke path with even -odd rule ('b*).\n");
        }
        else if (!strcmp(token, "n"))
        {
          // End path without filling or stroking (new path)
          cairo_new_path(cr);

          if (g_verbose)
            printf("DEBUG: New path ('n').\n");
        }
        else if (!strcmp(token, "W*"))
        {
          // Set the clipping path using the even-odd rule
          cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
          cairo_clip(cr);
          cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);

          if (g_verbose)
            printf("DEBUG: Set clipping path with even-odd rule ('W*').\n");
        }
        else if (!strcmp(token, "W"))
        {
          cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
          cairo_clip(cr);

          if (g_verbose)
            printf("DEBUG: Set clipping path with non-zero winding rule ('W').\n");
        }
        else if (!strcmp(token, "gs"))
        {
          if (g_verbose)
            printf("DEBUG: gs token came, implementation will be later");
        }



      // Clear the operand stack for the next command
      num_operands = 0;
    }
  }
  puts("--- End of Content Stream -- \n");
}





int                                   // 0 - Exit Status
main( int argc,                       // I - Number of command-line args
      char *argv[])                   // I - Command-line arguments
{
  pdfio_file_t  *pdf;
  pdfio_obj_t   *page;
  pdfio_rect_t  mediabox;
  cairo_surface_t *surface;
  cairo_t         *cr;
  int           status = 0;
  char *input_filename = NULL;
  char *output_filename = NULL;
  int opt;
  int pagenum = 1;
  int dpi = 72;
  int analyze_mode = 0;

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--analyze"))
    {
      analyze_mode = 1;

      for (int j = i; j < argc - 1; j++)
      {
        argv[j] = argv[j + 1];
      }
      argc--;

      break;
    }
  }
  

  while ((opt = getopt(argc, argv, "o:p:r:v")) != -1 )
  {
    switch (opt)
    {
      case 'o': 
        output_filename = optarg;
        break;

      case 'p':
        pagenum = atoi(optarg);
          break;

      case 'r':
          dpi = atoi(optarg);
          break;

      case 'v':
          g_verbose = 1;
          break;

      default: /* '?' */
        fprintf(stderr, "Usage: %s [-o output.png] input.pdf\n", argv[0]);
        return (1);
    }
  }

  if (optind < argc )
  {
    input_filename = argv[optind];
  }

  if (input_filename == NULL || (!analyze_mode && output_filename == NULL))
  {
    fprintf(stderr, "Usage: %s [-o output.png] input.pdf\n", argv[0]);
    return (1);
  }


  // Open the PDF file
  if ((pdf = pdfioFileOpen(input_filename, NULL, NULL, NULL, NULL)) == NULL)
    return (1);

  // Get the first Page...
  if ((page = pdfioFileGetPage(pdf, pagenum - 1)) == NULL)
  {
    fprintf(stderr, "Unable to get page 1 from '%s'.\n", argv[1]);
    pdfioFileClose(pdf);
    return (1);
  }

  // Get the page size....
  pdfioPageGetMediaBox(page, &mediabox);

  pdfio_dict_t *page_dict = pdfioObjGetDict(page);
  pdfio_obj_t *resources = NULL;
  if (page_dict)
    resources = pdfioDictGetObj(page_dict, "Resources");
  if (!resources)
  {
    fprintf(stderr, "ERROR: Unable to get page resources.\n");
    // Consider cleanup and exit here
  }


  if (g_verbose)
    printf("Page 1 is %.2f wide and %.2f high.\n", mediabox.x2 - mediabox.x1, mediabox.y2-mediabox.y1);

  double scale = dpi / 72.0;

  // Create a cairo surface and rendering context...
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)((mediabox.x2 - mediabox.x1) * scale ), (int)((mediabox.y2 - mediabox.y1) * scale ));
  cr  = cairo_create(surface);
  
  cairo_scale(cr, scale, scale);

  // Flip the coordinate system to match PDF (origin at bottom-left)
  cairo_translate(cr, 0, mediabox.y2 - mediabox.y1);
  cairo_scale(cr, 1.0, -1.0);

  // Initialize the graphics state stack
  gstack[0].fill_rgb[0] = 0.0; // Black
  gstack[0].fill_rgb[1] = 0.0;
  gstack[0].fill_rgb[2] = 0.0;
  gstack[0].stroke_rgb[0] = 0.0;
  gstack[0].stroke_rgb[1] = 0.0;
  gstack[0].stroke_rgb[2] = 0.0;
  gstack[0].line_width = 1.0;
  gstack_ptr = 0;


  // Open the page's content stream..
  pdfio_stream_t *st = pdfioPageOpenStream(page, 0, true);


  if (analyze_mode)
  {
    // --- ANALYSIS MODE ---
    if (st)
      analyze_content_stream(st);

    // Print the summary
    printf("--- Operator Analysis Summary ---\n");
    for (int i = 0; i < num_op_counts; i++)
      printf(" Fount operator '%s' : '%d' times \n", op_counts[i].name, op_counts[i].count);

    // Free the memory we allocated
    free(op_counts);

  }
  else
  {
    // --- RENDER MODE (Original Code ) ---
    // Initialize graphics state, paint background,etc.
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White Background
    cairo_paint(cr);

    if (st)
    {
      process_content_stream(cr, st, resources);
    }

    // Save to PNG..
    if (cairo_surface_write_to_png(surface, output_filename) != CAIRO_STATUS_SUCCESS)
    {
      fprintf(stderr, "Unable to write to '%s'.\n", output_filename);
      status = 1;
    }
    else
    {
  
      if (g_verbose)
        printf("Wrote blank page to '%s'.\n", output_filename);
    }

  }

  if(st)
    pdfioStreamClose(st);
  
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  pdfioFileClose(pdf);

  return (status);
}

