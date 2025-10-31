#include "interpreter.h"
#include "cairo_device.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use the global verbose flag defined in the main file
extern int g_verbose;


#define MAX_OPERANDS 256
static operand_t operand_stack[MAX_OPERANDS];
static int operand_stack_ptr = 0;


// --- Operator Handler Functions ---
// Each function handles the logic for a single PDF operator.

static void handle_q(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator q (Save State)\n");
    device_save_state(dev);
}

static void handle_Q(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator Q (Restore State)\n");
    device_restore_state(dev);
}

static void handle_BT(p2c_device_t *dev, pdfio_obj_t *resources) {
    device_begin_text(dev);
}

static void handle_ET(p2c_device_t *dev, pdfio_obj_t *resources) {
    device_end_text(dev);
}

static void handle_Td(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 2 && operand_stack[0].type == OP_TYPE_NUMBER && operand_stack[1].type == OP_TYPE_NUMBER) {
        if (g_verbose) printf("DEBUG: Operator Td (Move Text) with args (%f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number);
        device_move_text_cursor(dev, operand_stack[0].value.number, operand_stack[1].value.number);
    }
}

static void handle_TD(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 2 && operand_stack[0].type == OP_TYPE_NUMBER && operand_stack[1].type == OP_TYPE_NUMBER) {
        if (g_verbose) printf("DEBUG: Operator TD (Move text and Set Leading) with args (%f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number);
        device_set_text_leading(dev, -operand_stack[1].value.number);
        device_move_text_cursor(dev, operand_stack[0].value.number, operand_stack[1].value.number);
    }
}

static void handle_T_star(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator T* (Next Line)\n");
    device_next_line(dev);
}

static void handle_Tm(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 6) { // Assume all are numbers for brevity
        device_set_text_matrix(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number, operand_stack[4].value.number, operand_stack[5].value.number);
    }
}

static void handle_Tf(p2c_device_t *dev, pdfio_obj_t *resources) {
    printf("\n\n>>>>>>>>>>> TF <<<<<<<<<<<<<\n\n");
    if (operand_stack_ptr == 2 && operand_stack[0].type == OP_TYPE_NAME && operand_stack[1].type == OP_TYPE_NUMBER) {
        if (g_verbose) printf("DEBUG: Operator Tf (Set Font) with name %s and size %f\n", operand_stack[0].value.name, operand_stack[1].value.number);
        device_set_font(dev, operand_stack[0].value.name + 1, operand_stack[1].value.number); // +1 to skip leading '/'
    }
}

static void handle_Tj(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_STRING) {
        if (g_verbose) printf("DEBUG: Operator Tj (Show Text) with string \"%s\"\n", operand_stack[0].value.string);
        device_show_text(dev, operand_stack[0].value.string);
    }
}

static void handle_TJ(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr > 0) {
        device_show_text_kerning(dev, operand_stack, operand_stack_ptr);
    }
}

static void handle_w(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NUMBER) {
        if (g_verbose) printf("DEBUG: Operator w (Set Line Width) with arg %f\n", operand_stack[0].value.number);
        device_set_line_width(dev, operand_stack[0].value.number);
    }
}

static void handle_rg(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 3) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator rg (Set Fill RGB) with args (%f, %f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number);
        device_set_fill_rgb(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number);
    }
}

static void handle_RG(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 3) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator RG (Set Stroke RGB) with args (%f, %f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number);
        device_set_stroke_rgb(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number);
    }
}

static void handle_g(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NUMBER) {
        if (g_verbose) printf("DEBUG: Operator g (Set Fill Gray) with arg %f\n", operand_stack[0].value.number);
        device_set_fill_gray(dev, operand_stack[0].value.number);
    }
}

static void handle_G(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NUMBER) {
        if (g_verbose) printf("DEBUG: Operator G (Set Stroke Gray) with arg %f\n", operand_stack[0].value.number);
        device_set_stroke_gray(dev, operand_stack[0].value.number);
    }
}

static void handle_m(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 2) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator m (Move To) with args (%f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number);
        device_move_to(dev, operand_stack[0].value.number, operand_stack[1].value.number);
    }
}

static void handle_l(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 2) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator l (Line To) with args (%f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number);
        device_line_to(dev, operand_stack[0].value.number, operand_stack[1].value.number);
    }
}

static void handle_c(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 6) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator c (Curve To) with args (%f,%f %f,%f %f,%f)\n", operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number, operand_stack[4].value.number, operand_stack[5].value.number);
        device_curve_to(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number, operand_stack[4].value.number, operand_stack[5].value.number);
    }
}

static void handle_re(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 4) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator re (Rectangle) with args (%f, %f, %f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number);
        device_rectangle(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number);
    }
}

static void handle_h(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator h (Close Path)\n");
    device_close_path(dev);
}

static void handle_S(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator S (Stroke Path)\n");
    device_stroke(dev);
}

static void handle_f(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator f (Fill Path)\n");
    device_fill(dev);
}

static void handle_f_star(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator f* (Fill Path Even-Odd)\n");
    device_fill_even_odd(dev);
}

static void handle_B(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator B (Fill and Stroke Path)\n");
    device_fill_preserve(dev);
    device_stroke(dev);
}

static void handle_B_star(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator B* (Fill and Stroke Path Even-Odd)\n");
    device_fill_preserve_even_odd(dev);
    device_stroke(dev);
}

static void handle_b(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator b (Close, Fill, and Stroke Path)\n");
    device_close_path(dev);
    device_fill_preserve(dev);
    device_stroke(dev);
}

static void handle_b_star(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator b* (Close, Fill, and Stroke Path Even-Odd)\n");
    device_close_path(dev);
    device_fill_preserve_even_odd(dev);
    device_stroke(dev);
}

static void handle_n(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator n (New Path / No-Op)\n");
    // This is a no-op for our device, as paths are implicitly started.
}

static void handle_W(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator W (Clip Path)\n");
    device_clip(dev);
}

static void handle_W_star(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (g_verbose) printf("DEBUG: Operator W* (Clip Path Even-Odd)\n");
    device_clip_even_odd(dev);
}

static void handle_gs(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NAME) {
        if (g_verbose) printf("DEBUG: Operator gs (Set Graphics State) with name %s\n", operand_stack[0].value.name);
        device_set_graphics_state(dev, resources, operand_stack[0].value.name + 1); // +1 to skip leading '/'
    }
}

static void handle_cs(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NAME) {
        if (g_verbose) printf("DEBUG: Operator cs (Set fill Color Space) with name %s\n", operand_stack[0].value.name);
    }
}

static void handle_CS(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NAME) {
        if (g_verbose) printf("DEBUG: Operator CS (Set Stroke Color Space) with name %s \n", operand_stack[0].value.name);
    }
}

static void handle_k(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 4) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator k (Set Fill CMYK) with args (%f, %f, %f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number);
        device_set_fill_cmyk(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number);
    }
}

static void handle_K(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 4) { // Assume numbers
        if (g_verbose) printf("DEBUG: Operator K (Set Stroke CMYK) with args (%f, %f, %f, %f)\n", operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number);
        device_set_stroke_cmyk(dev, operand_stack[0].value.number, operand_stack[1].value.number, operand_stack[2].value.number, operand_stack[3].value.number);
    }
}


static void handle_Tr(p2c_device_t *dev, pdfio_obj_t *resources) {
    if (operand_stack_ptr == 1 && operand_stack[0].type == OP_TYPE_NUMBER) {
        int mode = (int)operand_stack[0].value.number;
        if (g_verbose) printf("DEBUG: Operator Tr (Set Text Rendering Mode) with mode %d\n", mode);
        // This is a placeholder for the actual device function you'll write
        // For now, let's just imagine it exists
        // device_set_text_rendering_mode(dev, mode);
        // Since our state is in the internal header, we can set it directly
        // from here for simplicity, but a dedicated function is cleaner.
        // For now, let's assume we don't have that function yet. We'll add it to graphics_state_t.
        // This is a simplified approach, a dedicated function in cairo_device.c would be better.
        device_set_text_rendering_mode(dev, mode);
    }
}

// --- Dispatch Table and Logic ---

// 1. Define a type for our handler functions
typedef void (*pdf_operator_handler_t)(p2c_device_t *dev, pdfio_obj_t *resources);

// 2. Define the structure for our lookup table entries
typedef struct {
    const char *name;
    pdf_operator_handler_t handler;
} pdf_operator_t;

// 3. Create the lookup table.
// IMPORTANT: This table MUST be sorted alphabetically by operator name for bsearch to work.
static const pdf_operator_t operator_table[] = {
    {"B", handle_B},
    {"B*", handle_B_star},
    {"BT", handle_BT},
    {"CS", handle_CS},
    {"ET", handle_ET},
    {"G", handle_G},
    {"K", handle_K},
    {"Q", handle_Q},
    {"RG", handle_RG},
    {"S", handle_S},
    {"T*", handle_T_star},
    {"TD", handle_TD},
    {"TJ", handle_TJ},
    {"Td", handle_Td},       // Correct Position
    {"Tf", handle_Tf},       // Correct Position
    {"Tj", handle_Tj},       // Correct Position
    {"Tm", handle_Tm},       // Correct Position
    {"Tr", handle_Tr},       // Correct Position
    {"W", handle_W},
    {"W*", handle_W_star},
    {"b", handle_b},
    {"b*", handle_b_star},
    {"c", handle_c},
    {"cs", handle_cs},
    {"f", handle_f},
    {"f*", handle_f_star},
    {"g", handle_g},
    {"gs", handle_gs},
    {"h", handle_h},
    {"k", handle_k},
    {"l", handle_l},
    {"m", handle_m},
    {"n", handle_n},
    {"q", handle_q},
    {"re", handle_re},
    {"rg", handle_rg},
    {"w", handle_w},
};

static const size_t operator_table_size = sizeof(operator_table) / sizeof(operator_table[0]);

// 4. Create a comparison function for bsearch
static int compare_operators(const void *a, const void *b) {
    const char *token = (const char *)a;
    const pdf_operator_t *op = (const pdf_operator_t *)b;
    return strcmp(token, op->name);
}

void process_content_stream(p2c_device_t *dev, pdfio_stream_t *st, pdfio_obj_t *resources)
{
  char token[1024];
  device_set_resources(dev, resources);
  // Reset the operand stacks
  operand_stack_ptr = 0;

  while (pdfioStreamGetToken(st, token, sizeof(token)))
  {
    if (g_verbose) fprintf(stderr, "DEBUG: Token: '%s'\n", token);
    if (isdigit(token[0]) || (token[0] == '-' && isdigit(token[1])) || (token[0] == '+' && isdigit(token[1])) || token[0] == '.')
    {
        // We have a number, push it on the operand stack
        if (operand_stack_ptr < MAX_OPERANDS) {
            operand_t *op = &operand_stack[operand_stack_ptr++];
            op->type = OP_TYPE_NUMBER;
            op->value.number = atof(token);
            if (g_verbose) printf("DEBUG: Pushed number: %f\n", op->value.number);
        }
    }
    else if (token[0] == '/')
    {
        // We have a name, push it on the operand stack
        if (operand_stack_ptr < MAX_OPERANDS) {
            operand_t *op = &operand_stack[operand_stack_ptr++];
            op->type = OP_TYPE_NAME;
            strcpy(op->value.name, token);
            if (g_verbose) printf("DEBUG: Pushed name: %s\n", op->value.name);
        }
    }
    else if (token[0] == '(')
    {
        // We have a string, push it on the operand stack
        if (operand_stack_ptr < MAX_OPERANDS) {
            operand_t *op = &operand_stack[operand_stack_ptr++];
            op->type = OP_TYPE_STRING;

            size_t len = strlen(token);

            // Check if string ends with ')'
            if (len > 1 && token[len - 1] == ')') {
                // String is complete: (text)
                strncpy(op->value.string, token + 1, len - 2);
                op->value.string[len - 2] = '\0';
            } else {
                // String doesn't end with ')': (text
                strncpy(op->value.string, token + 1, len - 1);
                op->value.string[len - 1] = '\0';
            }

            if (g_verbose) fprintf(stderr, "DEBUG: Pushed string: \"%s\"\n", op->value.string);
        }
    }
    // Note: We are ignoring array tokens `[` and `]` for now.
    // The operands for TJ will just be pushed onto the stack in order.
    else if (token[0] != '[' && token[0] != ']')
    {
      // We have an operator, find it in the table and execute it.
      const pdf_operator_t *op = bsearch(token, operator_table, operator_table_size, sizeof(pdf_operator_t), compare_operators);

      if (op) {
        op->handler(dev, resources);
      } else {
        if (g_verbose) printf("DEBUG: Unhandled operator: %s\n", token);
      }

      // Clear the operand stack for the next command
      operand_stack_ptr = 0;
    }
  }
}
