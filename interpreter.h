#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "cairo_device.h"
#include "pdfio.h"

// Defines the types of operands that can be on the stack
typedef enum {
  OP_TYPE_NONE,
  OP_TYPE_NUMBER,
  OP_TYPE_NAME,
  OP_TYPE_STRING
} operand_type_t;

// Defines a single generic operand on the stack
typedef struct {
  operand_type_t type;
  union {
    double number;
    char name[1024];
    char string[1024];
  } value;
} operand_t;


/**
 * @brief Processes a PDF content stream and uses a device to render it
 *
 * @param[in] dev The rendering device to draw with
 * @param[in] st The PDF content stream to read the commands from.
 * @param[in] resources The page's resource dictionary.
 */
void process_content_stream(p2c_device_t *dev, pdfio_stream_t *st, pdfio_obj_t *resources);

#endif // INTERPRETER_H
