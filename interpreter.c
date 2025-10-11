#include "interpreter.h"
#include "cairo_device.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use the global verbose flag defined in the main file
extern int g_verbose;

#define MAX_OPERANDS 256
static double num_operands[MAX_OPERANDS];
static int num_operands_ptr = 0;
static char name_operands[MAX_OPERANDS][1024];
static int name_operands_ptr = 0;
static char string_operands[MAX_OPERANDS][1024];
static int string_operands_ptr = 0;

void process_content_stream(p2c_device_t *dev, pdfio_stream_t *st, pdfio_obj_t *resources)
{
  char token[1024];
  device_set_resources(dev, resources);
  // Reset the operand stacks
  num_operands_ptr = 0;
  name_operands_ptr = 0;

  while (pdfioStreamGetToken(st, token, sizeof(token)))
  {
    if (isdigit(token[0]) || token[0] == '.' || token[0] == '-' || token[0] == '+')
    {
      // We have a number, push it on the numeric operand stack
      if (num_operands_ptr < MAX_OPERANDS)
      {
        double val = atof(token);
        if (g_verbose)
          printf("DEBUG: Pushed number: %f\n", val);
        num_operands[num_operands_ptr++] = val;
      }
    }
    else if (token[0] == '/')
    {
      // We have a name, push it on the name operand stack
      if (name_operands_ptr < MAX_OPERANDS)
      {
        if (g_verbose)
          printf("DEBUG: Pushed name: %s\n", token);
        strcpy(name_operands[name_operands_ptr++], token);
      }
    }
    else if (token[0] == '(' )
    {
      // We have a string, push it on the string operand stack
      if (string_operands_ptr < MAX_OPERANDS)
      {
        // Copy the string, removing the leading '(' and trailing ')'
        size_t len = strlen(token);
        if (len > 1 && token[len - 1] == ')')
        {
          strncpy(string_operands[string_operands_ptr], token + 1, len - 2);
          string_operands[string_operands_ptr][len - 2] = '\0'; // Null-terminate
          if (g_verbose) printf("DEBUG: Pushed string: \"%s\"\n", string_operands[string_operands_ptr]);
          string_operands_ptr++;
        }
      }
    }
    else
    {
      // We have a command, process it...
      if (!strcmp(token, "q"))
      {
        if (g_verbose) printf("DEBUG: Operator q (Save State)\n");
        device_save_state(dev);
      }
      else if (!strcmp(token, "Q"))
      {
        if (g_verbose) printf("DEBUG: Operator Q (Restore State)\n");
        device_restore_state(dev);
      }
      else if (!strcmp(token, "BT"))
      {
        device_begin_text(dev);
      }
      else if (!strcmp(token, "ET"))
      {
        device_end_text(dev);
      }
      else if (!strcmp(token, "Td"))
      {
        if (num_operands_ptr == 2)
        {
          if (g_verbose) printf("DEBUG: Operator Td (Move Text) with args (%f, %f)\n",num_operands[0], num_operands[1]);
          device_move_text_cursor(dev, num_operands[0], num_operands[1]);
        }
      }
      else if (!strcmp(token, "TD"))
      {
        if (num_operands_ptr == 2)
        {
          if (g_verbose) printf("DEBUG: Operator TD (Move text and Set Leading) with args (%f, %f)\n", num_operands[0], num_operands[1]);
          device_set_text_leading(dev, -num_operands[1]);
          device_move_text_cursor(dev, num_operands[0], num_operands[1]);
        }
      }
      else if (!strcmp(token, "T*"))
      {
        if (g_verbose) printf("DEBUG: Operator T* (Next Line)\n");
        device_next_line(dev);
      }
      else if (!strcmp(token, "Tm"))
      {
        if (num_operands_ptr == 6)
        {
          device_set_text_matrix(dev, num_operands[0], num_operands[1], num_operands[2], num_operands[3], num_operands[4], num_operands[5]);
        }
      }
      else if (!strcmp(token, "Tf"))
      {
        if (name_operands_ptr == 1 && num_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator Tf (Set Font) with name %s and size %f\n", name_operands[0], num_operands[0]);
          // The name operand includes the leading '/', so we pass name_operands[0] + 1 to skip it
          device_set_font(dev, name_operands[0] + 1, num_operands[0]);
        }
      }
      else if (!strcmp(token, "Tj"))
      {
        if (string_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator Tj (Show Text) with string \"%s\"\n", string_operands[0]);
          device_show_text(dev, string_operands[0]);
        }
      }
      else if (!strcmp(token, "w"))
      {
        if (num_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator w (Set Line Width) with arg %f\n", num_operands[0]);
          device_set_line_width(dev, num_operands[0]);
        }
      }
      else if (!strcmp(token, "rg"))
      {
        if (num_operands_ptr == 3)
        {
          if (g_verbose) printf("DEBUG: Operator rg (Set Fill RGB) with args (%f, %f, %f)\n", num_operands[0], num_operands[1], num_operands[2]);
          device_set_fill_rgb(dev, num_operands[0], num_operands[1], num_operands[2]);
        }
      }
      else if (!strcmp(token, "RG"))
      {
        if (num_operands_ptr == 3)
        {
          if (g_verbose) printf("DEBUG: Operator RG (Set Stroke RGB) with args (%f, %f, %f)\n", num_operands[0], num_operands[1], num_operands[2]);
          device_set_stroke_rgb(dev, num_operands[0], num_operands[1], num_operands[2]);
        }
      }
      else if (!strcmp(token, "g"))
      {
        if (num_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator g (Set Fill Gray) with arg %f\n", num_operands[0]);
          device_set_fill_gray(dev, num_operands[0]);
        }
      }
      else if (!strcmp(token, "G"))
      {
        if (num_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator G (Set Stroke Gray) with arg %f\n", num_operands[0]);
          device_set_stroke_gray(dev, num_operands[0]);
        }
      }
      else if (!strcmp(token, "m"))
      {
        if (num_operands_ptr == 2)
        {
          if (g_verbose) printf("DEBUG: Operator m (Move To) with args (%f, %f)\n", num_operands[0], num_operands[1]);
          device_move_to(dev, num_operands[0], num_operands[1]);
        }
      }
      else if (!strcmp(token, "l"))
      {
        if (num_operands_ptr == 2)
        {
          if (g_verbose) printf("DEBUG: Operator l (Line To) with args (%f, %f)\n", num_operands[0], num_operands[1]);
          device_line_to(dev, num_operands[0], num_operands[1]);
        }
      }
      else if (!strcmp(token, "c"))
      {
        if (num_operands_ptr == 6)
        {
          if (g_verbose) printf("DEBUG: Operator c (Curve To) with args (%f,%f %f,%f %f,%f)\n", num_operands[0], num_operands[1], num_operands[2], num_operands[3], num_operands[4], num_operands[5]);
          device_curve_to(dev, num_operands[0], num_operands[1], num_operands[2], num_operands[3], num_operands[4], num_operands[5]);
        }
      }
      else if (!strcmp(token, "re"))
      {
        if (num_operands_ptr == 4)
        {
          if (g_verbose) printf("DEBUG: Operator re (Rectangle) with args (%f, %f, %f, %f)\n", num_operands[0], num_operands[1], num_operands[2], num_operands[3]);
          device_rectangle(dev, num_operands[0], num_operands[1], num_operands[2], num_operands[3]);
        }
      }
      else if (!strcmp(token, "h"))
      {
        if (g_verbose) printf("DEBUG: Operator h (Close Path)\n");
        device_close_path(dev);
      }
      else if (!strcmp(token, "S"))
      {
        if (g_verbose) printf("DEBUG: Operator S (Stroke Path)\n");
        device_stroke(dev);
      }
      else if (!strcmp(token, "f"))
      {
        if (g_verbose) printf("DEBUG: Operator f (Fill Path)\n");
        device_fill(dev);
      }
      else if (!strcmp(token, "f*"))
      {
        if (g_verbose) printf("DEBUG: Operator f* (Fill Path Even-Odd)\n");
        device_fill_even_odd(dev);
      }
      else if (!strcmp(token, "B"))
      {
        if (g_verbose) printf("DEBUG: Operator B (Fill and Stroke Path)\n");
        device_fill_preserve(dev);
        device_stroke(dev);
      }
      else if (!strcmp(token, "B*"))
      {
        if (g_verbose) printf("DEBUG: Operator B* (Fill and Stroke Path Even-Odd)\n");
        device_fill_preserve_even_odd(dev);
        device_stroke(dev);
      }
      else if (!strcmp(token, "b"))
      {
        if (g_verbose) printf("DEBUG: Operator b (Close, Fill, and Stroke Path)\n");
        device_close_path(dev);
        device_fill_preserve(dev);
        device_stroke(dev);
      }
      else if (!strcmp(token, "b*"))
      {
        if (g_verbose) printf("DEBUG: Operator b* (Close, Fill, and Stroke Path Even-Odd)\n");
        device_close_path(dev);
        device_fill_preserve_even_odd(dev);
        device_stroke(dev);
      }
      else if (!strcmp(token, "n"))
      {
        if (g_verbose) printf("DEBUG: Operator n (New Path / No-Op)\n");
        // This is a no-op for our device, as paths are implicitly started.
      }
      else if (!strcmp(token, "W"))
      {
        if (g_verbose) printf("DEBUG: Operator W (Clip Path)\n");
        device_clip(dev);
      }
      else if (!strcmp(token, "W*"))
      {
        if (g_verbose) printf("DEBUG: Operator W* (Clip Path Even-Odd)\n");
        device_clip_even_odd(dev);
      }
      else if (!strcmp(token, "gs"))
      {
        if (name_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator gs (Set Graphics State) with name %s\n", name_operands[0]);
          device_set_graphics_state(dev, resources, name_operands[0] + 1);
        }
      }
      else if (!strcmp(token, "cs"))
      {
        if (name_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator cs (Set fill Color Space) with name %s\n", name_operands[0]);
          // Note: For now we only support DeviceGray and DeviceRGB
          if (!strcmp(name_operands[0], "/DeviceRGB"))
          {
            // In a full implmentation, you'd call a device function here.
            // For now, our color-setting operators handle this implicitly.
            // This block is a placeholder for future color management.
          }
        }
      }
      else if (!strcmp(token, "CS"))
      {
        if (name_operands_ptr == 1)
        {
          if (g_verbose) printf("DEBUG: Operator CS (Set Stroke Color Space) with name %s \n", name_operands[0]);
          if (!strcmp(name_operands[0], "/DeviceRGB"))
          {
            // In a full implementation, you'd call a device function here.
            // For now, our color-setting operators handle this implictily.
            // This block is a placeholder for future color management.
          }
        }
      }
      else if (!strcmp(token, "k"))
      {
        if (num_operands_ptr == 4)
        {
          if (g_verbose) printf("DEBUG: Operator k (Set Fill CMYK) with args (%f, %f, %f, %f)\n", num_operands[0], num_operands[1], num_operands[2], num_operands[3]);
          device_set_fill_cmyk(dev, num_operands[0], num_operands[1], num_operands[2], num_operands[3]);
        }
      }
      else if (!strcmp(token, "K"))
      {
        if (num_operands_ptr == 4)
        {
          if (g_verbose) printf("DEBUG: Operator K (Set Stroke CMYK) with args (%f, %f, %f, %f)\n",num_operands[0], num_operands[1], num_operands[2], num_operands[3]);
          device_set_stroke_cmyk(dev, num_operands[0], num_operands[1], num_operands[2], num_operands[3]);
        }
      }
      else
      {
        if (g_verbose)
          printf("DEBUG: Unhandled operator: %s\n", token);
      }

      // Clear the operand stacks for the next command
      num_operands_ptr = 0;
      name_operands_ptr = 0;
      string_operands_ptr = 0;
    }
  }
}
