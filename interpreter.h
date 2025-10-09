#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "cairo_device.h"
#include "pdfio.h"

/**
 * @brief Processes a PDF content stream and uses a device to render it
 *
 * @param[in] dev The rendering device to draw with
 * @param[in] st The PDF content stream to read the commands from.
 * @param[in] resources The page's resource dictionary.
 */

void process_content_stream(p2c_device_t *dev, pdfio_stream_t *st, pdfio_obj_t *resources);

#endif
