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
  operand_type_t  type;
  size_t	        length;		// Number of bytes in value.string
  union 
  {
    double          number;
    char            name[1024];
    unsigned char   string[1024];	  // Raw string bytes (may contain NULs)
  } value;
} operand_t;

// A content stream read completely into memory as a list of tokens
typedef struct pdf_content_s
{
  char		    *data;		// Tokens, each NUL-terminated
  size_t	    used,			// Bytes used in data
		          alloc;		// Bytes allocated for data
  size_t	    *starts; 	// Offset of each token in data
  size_t	    num_tokens,   // Number of tokens
		          alloc_tokens; // Number of offsets allocated
} pdf_content_t;

// Context required while running a content stream
typedef struct parser_context_s
{
  p2c_device_t  *device;
  pdfio_dict_t  *resources;		  // Resources for names used by operators
  bool		      ignore_color;		// Running an uncolored Type 3 glyph (d1)

  operand_t     *operands;
  size_t        num_operands;
  size_t        operand_capacity;
} parser_context_t;


/**
 * @brief Processes a PDF content stream and uses a device to render it
 *
 * @param[in] dev The rendering device to draw with
 * @param[in] page_data The page whose content streams are drawn.
 */
void process_content_stream(p2c_device_t *dev, pdfrip_page_t *page_data);

// Read the content stream of a form XObject or Type 3 glyph into memory
bool pdf_content_load(pdf_content_t *content, pdfio_obj_t *obj);
void pdf_content_free(pdf_content_t *content);

// Run in-memory content with the given resources (forms, Type 3 glyphs)
void pdf_content_run(p2c_device_t *dev, const pdf_content_t *content, pdfio_dict_t *resources);
		
#endif // PARSER_H
