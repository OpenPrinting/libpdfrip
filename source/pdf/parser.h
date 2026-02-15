//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef PARSER_H
#define PARSER_H

#include <pdfio.h>
#include "pdfops-private.h"

typedef struct pdfrip_page_s pdfrip_page_t;
typedef struct cairo_device_s p2c_device_t;

// Defines the types of operands that can be on the stack
typedef enum operand_type_s
{
  OP_TYPE_NONE,
  OP_TYPE_NUMBER,
  OP_TYPE_NAME,
  OP_TYPE_STRING
} operand_type_t;

// Defines a single generic operand on the stack
typedef struct operand_s
{
  operand_type_t type;
  union 
  {
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
void process_content_stream(p2c_device_t *dev, pdfrip_page_t *page_data);
		

#endif // PARSER_H
