//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cairo-private.h"
#include <jpeglib.h>
#include <setjmp.h>

typedef struct p2c_jpeg_error_s
{
  struct jpeg_error_mgr pub;
  jmp_buf               jmp;
} p2c_jpeg_error_t;

static cairo_user_data_key_t p2c_surface_data_key;

static void
p2c_jpeg_error_exit(j_common_ptr cinfo)
{
  p2c_jpeg_error_t *error = (p2c_jpeg_error_t *)cinfo->err;

  (*cinfo->err->output_message)(cinfo);
  longjmp(error->jmp, 1);
}

static cairo_surface_t *
decode_jpeg_surface(const unsigned char *data, size_t length)
{
  struct jpeg_decompress_struct cinfo;
  p2c_jpeg_error_t              jerr;
  cairo_surface_t               *surface = NULL;
  unsigned char                 *surface_data = NULL;
  JSAMPLE                       *buffer = NULL;
  int                           stride;
  int                           width, height, components;

  cinfo.err             = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit   = p2c_jpeg_error_exit;

  if (setjmp(jerr.jmp))
  {
    jpeg_destroy_decompress(&cinfo);
    free(buffer);
    free(surface_data);
    return (NULL);
  }

  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char *)data, length);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  width      = (int)cinfo.output_width;
  height     = (int)cinfo.output_height;
  components = cinfo.output_components;
  stride     = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width);

  surface_data = calloc((size_t)stride, cinfo.output_height);
  buffer = malloc((size_t)cinfo.output_width * cinfo.output_components);
  if (!surface_data)
  {
    jpeg_destroy_decompress(&cinfo);
    return (NULL);
  }
  else if (!buffer)
  {
    jpeg_destroy_decompress(&cinfo);
    free(surface_data);
    return (NULL);
  }

  while (cinfo.output_scanline < cinfo.output_height)
  {
    JSAMPROW  row = surface_data + (size_t)stride * cinfo.output_scanline;
    JSAMPROW  src = buffer;

    jpeg_read_scanlines(&cinfo, &src, 1);

    for (JDIMENSION x = 0; x < cinfo.output_width; x ++)
    {
      unsigned char *dst = row + x * 4;

      if (components == 3)
      {
        dst[0] = buffer[x * 3 + 2];
        dst[1] = buffer[x * 3 + 1];
        dst[2] = buffer[x * 3 + 0];
      }
      else if (components == 1)
      {
        dst[0] = buffer[x];
        dst[1] = buffer[x];
        dst[2] = buffer[x];
      }

      dst[3] = 255;
    }
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  free(buffer);

  surface = cairo_image_surface_create_for_data(surface_data,
                                                CAIRO_FORMAT_RGB24,
                                                width,
                                                height,
                                                stride);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
  {
    free(surface_data);
    cairo_surface_destroy(surface);
    return (NULL);
  }

  cairo_surface_set_user_data(surface, &p2c_surface_data_key, surface_data, free);
  cairo_surface_mark_dirty(surface);

  return (surface);
}

// --- Device LifeCycle Functions ---

//
// 'device_create()' - Initializes the Cairo rendering environment. 
// 		       Sets up the pixel surface, coordinate system, and 
// 		       initial graphics state
//

p2c_device_t*				  // O - pointer to initialized cairo structure
device_create(pdfrip_page_t *page, 	// I - Data related to PDF page
	      int dpi)			// I - target resolution for output image(DPI)
{
  // Allocate memory for the device structure and zero it out
  p2c_device_t *dev = calloc(1, sizeof(p2c_device_t));
  if (!dev)
  {
    // Allocation failed
    fprintf(stderr, "ERROR: Could not allocate memory for Cairo device.\n");
    return (NULL);
  }

  // Calculate scale factor based on target DPI (PDF base is 72 DPI)
  double scale = dpi / 72.0;

  // Determine pixel dimensions from the PDF MediaBox and scale
  double width = (page->mediaBox.x2 - page->mediaBox.x1) * scale;
  double height = (page->mediaBox.y2 - page->mediaBox.y1) * scale;

  if (g_verbose)
    printf("DEBUG: Creating Cairo surface: %.2fx%.2f pixels (scale: %.2f)\n", width, height, scale);

  // set Null values to 
  dev->num_fonts = 0;

  // Create the underlying Cairo image surface and context
  dev->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
  if (cairo_surface_status(dev->surface) != CAIRO_STATUS_SUCCESS)
  {
    free(dev);
    return NULL;
  }

  dev->cr = cairo_create(dev->surface);

  // Scale Cairo so 1 user unit = 1 PDF point (handled by scaling matrix)
  cairo_scale(dev->cr, scale, scale);

  // Flip Y axis (PDF 0,0 is bottom-left, Cairo is top-left)
  cairo_translate(dev->cr, 0, page->mediaBox.y2 - page->mediaBox.y1); 
  cairo_scale(dev->cr, 1.0, -1.0); 


  // Define and set the default initial graphics state
  dev->gstack[0] = (graphics_state_t)
  {
    .fill_rgb = {0.0, 0.0, 0.0},
    .stroke_rgb = {0.0, 0.0, 0.0},
    .line_width = 1.0,
    .fill_alpha = 1.0,
    .stroke_alpha = 1.0,
    .text_leading = 0.0,
    .font_size = 1.0,
    .text_rendering_mode = 0,
    .fill_colorspace = CS_DEVICE_GRAY,
    .stroke_colorspace = CS_DEVICE_GRAY};

  // Initialize text matrices to identity
  cairo_matrix_init_identity(&dev->gstack[0].text_matrix);
  cairo_matrix_init_identity(&dev->gstack[0].text_line_matrix);
  dev->gstack_ptr = 0;

  // Initialize default encoding (WinAnsi fallback) to identity
  for (int i=0; i<256; i++) 
    dev->gstack[0].encoding[i] = i;

  // Prepare the surface with a default white background
  cairo_set_source_rgb(dev->cr, 1.0, 1.0, 1.0);
  cairo_paint(dev->cr);

  return (dev);
}

#include <cairo.h>
#include <pdfio.h>

void device_draw_image(p2c_device_t *dev, pdfio_obj_t *xobj)
{
  pdfio_dict_t	*dict = pdfioObjGetDict(xobj);
  pdfio_stream_t	*stream;
  unsigned char	*data = NULL;
  size_t	length = 0, capacity = 0;

  int width  = (int)pdfioDictGetNumber(dict, "Width");
  int height = (int)pdfioDictGetNumber(dict, "Height");
  int bpc    = (int)pdfioDictGetNumber(dict, "BitsPerComponent");

  const char *colorspace = pdfioDictGetName(dict, "ColorSpace");
  const char *filter     = pdfioDictGetName(dict, "Filter");

  fprintf(stderr, "DEBUG: Image %dx%d, bpc=%d, cs=%s, filter=%s\n",
          width, height, bpc, colorspace ? colorspace : "NULL",
          filter ? filter : "NONE");

  stream = pdfioObjOpenStream(xobj, !(filter && !strcmp(filter, "DCTDecode")));
  if (!stream)
  {
    fprintf(stderr, "ERROR: Cannot open image stream\n");
    return;
  }

  for (;;)
  {
    unsigned char	buffer[8192];
    ssize_t	bytes = pdfioStreamRead(stream, buffer, sizeof(buffer));

    if (bytes < 0)
    {
      fprintf(stderr, "ERROR: Cannot read image stream\n");
      free(data);
      pdfioStreamClose(stream);
      return;
    }

    if (bytes == 0)
      break;

    if (length + (size_t)bytes > capacity)
    {
      size_t		new_capacity = capacity ? capacity * 2 : 8192;
      unsigned char	*temp;

      while (new_capacity < length + (size_t)bytes)
        new_capacity *= 2;

      temp = realloc(data, new_capacity);
      if (!temp)
      {
        fprintf(stderr, "ERROR: Out of memory reading image stream\n");
        free(data);
        pdfioStreamClose(stream);
        return;
      }

      data     = temp;
      capacity = new_capacity;
    }

    memcpy(data + length, buffer, (size_t)bytes);
    length += (size_t)bytes;
  }

  pdfioStreamClose(stream);

  fprintf(stderr, "DEBUG: Got %zu bytes\n", length);

  // Temporary debug path for raw JPEG image data.
  if (filter && !strcmp(filter, "DCTDecode"))
  {
    cairo_surface_t *image;
    FILE *f = fopen("debug.jpg", "wb");
    if (f)
    {
      fwrite(data, 1, length, f);
      fclose(f);
      fprintf(stderr, "DEBUG: Saved image as debug.jpg\n");
    }

    image = decode_jpeg_surface(data, length);
    if (!image)
    {
      fprintf(stderr, "ERROR: Unable to decode JPEG image\n");
      free(data);
      return;
    }

    cairo_save(dev->cr);
    cairo_translate(dev->cr, 0.0, 1.0);
    cairo_scale(dev->cr, 1.0, -1.0);
    cairo_scale(dev->cr, 1.0 / width, 1.0 / height);
    cairo_set_source_surface(dev->cr, image, 0.0, 0.0);
    cairo_pattern_set_filter(cairo_get_source(dev->cr), CAIRO_FILTER_BEST);
    cairo_rectangle(dev->cr, 0.0, 0.0, width, height);
    cairo_fill(dev->cr);
    cairo_restore(dev->cr);

    cairo_surface_destroy(image);
    free(data);
    return;
  }

  fprintf(stderr, "WARNING: Unsupported image format\n");
  free(data);
}

//
// 'device_destroy()' - frees all allocated resources 
//

void 					  // O - Void 
device_destroy(p2c_device_t *dev)	// I - pointer to structure to be freed
{
  if (dev)
  {
    if (g_verbose)
      printf("DEBUG: Destroying Cairo device.\n");

    // Destroy the Cairo context and the image surface
    cairo_destroy(dev->cr);
    cairo_surface_destroy(dev->surface);

    // Thinking that each page might have different font thingy, so freeing it for now
    // But if error occur, will look into this.
    for (size_t i = 0; i < dev->num_fonts; i++) 
    {
      // Free the raw data and the Cairo face for the previous page
      free(dev->fonts[i]->data);
      cairo_font_face_destroy(dev->fonts[i]->cairo_face);
      free(dev->fonts[i]);
    }
    dev->num_fonts = 0;

    // Final step: free the device structure itself
    free(dev);
  }
}

//
// 'device_save_to_png()' - Saves the current rendered surface to a PNG file
//

void 						  // O - Void
device_save_to_png(p2c_device_t *dev, 		// I - Active Rendering context
		   const char *filename)	// I - File path where PNG will be saved
{
  if (g_verbose)
    printf("DEBUG: Writing surface to PNG: %s\n", filename);

  // Use Cairo's built-in utility to write the image surface to the filesystem
  if (cairo_surface_write_to_png(dev->surface, filename) != CAIRO_STATUS_SUCCESS)
  {
    // Report an error if the surface cannot be written
    fprintf(stderr, "ERROR: Unable to write PNG to '%s'.\n", filename);
  }
}
