//
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Image XObjects: decode the samples, convert them to cairo surfaces and
// hand them to the device (PDF 32000-1 8.9).
//

#include "pdfops-private.h"
#include "pdf-image-private.h"
#include <stdint.h>
#include <string.h>

static bool
set_kind(image_colorspace_t *cs, image_kind_t kind)
{
  cs->kind   = kind;
  cs->ncomps = kind == IMAGE_RGB ? 3 : kind == IMAGE_CMYK ? 4 : 1;

  return true;
}

static bool colorspace_from_array(pdfio_dict_t *resources, pdfio_array_t *array, image_colorspace_t *cs, int depth);

//
// 'colorspace_from_name()' - Parse a device color space name or a name in
//                            the /ColorSpace resources.
//

static bool
colorspace_from_name(pdfio_dict_t       *resources,
                     const char         *name,
                     image_colorspace_t *cs,
                     int                depth)
{
  pdfio_dict_t	*spaces;
  pdfio_array_t	*array;

  if (!name || depth > 4)
    return false;

  if (!strcmp(name, "DeviceGray") || !strcmp(name, "G"))
    return set_kind(cs, IMAGE_GRAY);
  else if (!strcmp(name, "DeviceRGB") || !strcmp(name, "RGB"))
    return set_kind(cs, IMAGE_RGB);
  else if (!strcmp(name, "DeviceCMYK") || !strcmp(name, "CMYK"))
    return set_kind(cs, IMAGE_CMYK);

  // A name from the /ColorSpace resources, e.g. /CS0 -> [/ICCBased 7 0 R].
  // Image XObjects should give the color space directly; some files don't.
  spaces = pdf_dict_get_dict(resources, "ColorSpace");

  if ((array = pdf_dict_get_array(spaces, name)) != NULL)
    return colorspace_from_array(resources, array, cs, depth + 1);

  return colorspace_from_name(resources, pdfioDictGetName(spaces, name), cs, depth + 1);
}

//
// 'colorspace_from_array()' - Parse an array color space such as
//                             [/ICCBased 7 0 R] or [/Indexed base hival lookup].
//

static bool
colorspace_from_array(pdfio_dict_t       *resources,
                      pdfio_array_t      *array,
                      image_colorspace_t *cs,
                      int                depth)
{
  const char *family = pdfioArrayGetName(array, 0);

  if (!family || depth > 4)
    return false;

  if (!strcmp(family, "ICCBased"))
  {
    // Use the number of components; the ICC profile itself is not applied
    int n = (int)pdfioDictGetNumber(pdfioObjGetDict(pdfioArrayGetObj(array, 1)), "N");

    return n == 1 ? set_kind(cs, IMAGE_GRAY) : n == 3 ? set_kind(cs, IMAGE_RGB) :
           n == 4 ? set_kind(cs, IMAGE_CMYK) : false;
  }
  else if (!strcmp(family, "CalGray"))
    return set_kind(cs, IMAGE_GRAY);
  else if (!strcmp(family, "CalRGB"))
    return set_kind(cs, IMAGE_RGB);
  else if (!strcmp(family, "Separation"))
  {
    // Approximation: the tint transform is not evaluated, the tint is shown
    // as gray ink
    cs->subtractive = true;
    return set_kind(cs, IMAGE_GRAY);
  }
  else if (!strcmp(family, "Indexed") || !strcmp(family, "I"))
  {
    image_colorspace_t	base = { 0 };
    pdfio_array_t	*base_array;
    const unsigned char	*table = NULL;
    unsigned char	*stream = NULL;
    size_t		table_len = 0, needed;
    bool		have_base;

    // The base space is a name or an array (possibly indirect)
    if ((base_array = pdfioArrayGetArray(array, 1)) == NULL)
      base_array = pdfioObjGetArray(pdfioArrayGetObj(array, 1));

    if (base_array)
      have_base = colorspace_from_array(resources, base_array, &base, depth + 1);
    else
      have_base = colorspace_from_name(resources, pdfioArrayGetName(array, 1), &base, depth + 1);

    if (!have_base || base.kind == IMAGE_INDEXED)
    {
      free(base.lookup);
      return false;
    }

    cs->kind        = IMAGE_INDEXED;
    cs->ncomps      = 1;
    cs->base        = base.kind;
    cs->base_ncomps = base.ncomps;
    cs->subtractive = base.subtractive;
    cs->hival       = (int)pdfioArrayGetNumber(array, 2);

    if (cs->hival < 0)
      cs->hival = 0;
    else if (cs->hival > 255)
      cs->hival = 255;

    // The palette is a string or a stream
    switch (pdfioArrayGetType(array, 3))
    {
      case PDFIO_VALTYPE_BINARY :
          table = pdfioArrayGetBinary(array, 3, &table_len);
          break;
      case PDFIO_VALTYPE_STRING :
          if ((table = (const unsigned char *)pdfioArrayGetString(array, 3)) != NULL)
            table_len = strlen((const char *)table);
          break;
      case PDFIO_VALTYPE_INDIRECT :
          table = stream = pdf_read_stream(pdfioArrayGetObj(array, 3), true, &table_len);
          break;
      default :
          break;
    }

    needed = (size_t)(cs->hival + 1) * (size_t)cs->base_ncomps;

    if ((cs->lookup = calloc(needed, 1)) != NULL && table)
      memcpy(cs->lookup, table, table_len < needed ? table_len : needed);

    free(stream);
    return cs->lookup != NULL;
  }

  // Lab, DeviceN, Pattern, ...
  return false;
}

static void
jpeg_error_exit(j_common_ptr cinfo)
{
  jpeg_error_t	*err = (jpeg_error_t *)cinfo->err;
  char		message[JMSG_LENGTH_MAX];

  (*cinfo->err->format_message)(cinfo, message);
  fprintf(stderr, "WARNING: Unable to decode JPEG image: %s\n", message);

  longjmp(err->jump, 1);
}

//
// 'decode_jpeg()' - Decode DCTDecode data with libjpeg into 8-bit samples.
//
// color_transform is /ColorTransform from /DecodeParms, or -1 when absent.
//

static bool
decode_jpeg(unsigned char *jpeg,
            size_t        size,
            int           color_transform,
            image_data_t  *img)
{
  struct jpeg_decompress_struct	cinfo;
  jpeg_error_t			err;
  unsigned char * volatile	pixels = NULL;

  cinfo.err = jpeg_std_error(&err.mgr);
  err.mgr.error_exit = jpeg_error_exit;

  if (setjmp(err.jump))
  {
    jpeg_destroy_decompress(&cinfo);
    free(pixels);
    return false;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, jpeg, (unsigned long)size);
  jpeg_read_header(&cinfo, TRUE);

  // /ColorTransform overrides what libjpeg guesses from the JPEG markers
  if (color_transform >= 0 && cinfo.num_components == 3)
    cinfo.jpeg_color_space = color_transform ? JCS_YCbCr : JCS_RGB;
  else if (color_transform >= 0 && cinfo.num_components == 4)
    cinfo.jpeg_color_space = color_transform ? JCS_YCCK : JCS_CMYK;

  // libjpeg converts YCbCr to RGB and YCCK to CMYK. CMYK is not inverted for
  // Adobe JPEGs: in PDF that is the job of the /Decode array (as in MuPDF).
  cinfo.out_color_space = cinfo.num_components == 4 ? JCS_CMYK :
                          cinfo.num_components == 1 ? JCS_GRAYSCALE : JCS_RGB;

  jpeg_start_decompress(&cinfo);

  if (cinfo.output_width > MAX_IMAGE_SIZE || cinfo.output_height > MAX_IMAGE_SIZE)
  {
    fprintf(stderr, "WARNING: JPEG image of %ux%u pixels is too large.\n",
            cinfo.output_width, cinfo.output_height);
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  img->width  = (int)cinfo.output_width;
  img->height = (int)cinfo.output_height;
  img->ncomps = cinfo.output_components;
  img->bpc    = 8;
  img->stride = (size_t)img->width * (size_t)img->ncomps;
  img->jpeg   = true;

  if ((pixels = malloc(img->stride * (size_t)img->height)) == NULL)
  {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  while (cinfo.output_scanline < cinfo.output_height)
  {
    JSAMPROW row = pixels + (size_t)cinfo.output_scanline * img->stride;

    jpeg_read_scanlines(&cinfo, &row, 1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  img->data = pixels;
  return true;
}

//
// 'load_samples()' - Read and decompress the samples of an image XObject.
//
// ncomps comes from the color space (JPEG data has its own); default_bpc is
// used when /BitsPerComponent is missing (1 for image masks).
//

static bool
load_samples(pdfio_obj_t  *obj,
             pdfio_dict_t *dict,
             int          ncomps,
             int          default_bpc,
             image_data_t *img)
{
  const char	*filter = pdfioDictGetName(dict, "Filter");
  pdfio_array_t	*filters = pdf_dict_get_array(dict, "Filter");
  size_t	size, needed;

  memset(img, 0, sizeof(*img));

  if (!filter && pdfioArrayGetSize(filters) == 1)
    filter = pdfioArrayGetName(filters, 0);

  if (filter && (!strcmp(filter, "DCTDecode") || !strcmp(filter, "DCT")))
  {
    pdfio_dict_t	*parms = pdf_dict_get_dict(dict, "DecodeParms");
    int		color_transform = -1;
    unsigned char	*jpeg = pdf_read_stream(obj, false, &size);
    bool		ret;

    if (pdfioDictGetType(parms, "ColorTransform") == PDFIO_VALTYPE_NUMBER)
      color_transform = (int)pdfioDictGetNumber(parms, "ColorTransform");

    ret = jpeg && decode_jpeg(jpeg, size, color_transform, img);
    free(jpeg);
    return ret;
  }

  if ((filter && strcmp(filter, "FlateDecode") && strcmp(filter, "Fl")) ||
      (!filter && pdfioArrayGetSize(filters) > 1))
  {
    fprintf(stderr, "WARNING: Images compressed with /%s are not supported yet.\n",
            filter ? filter : "(several filters)");
    return false;
  }

  img->width  = (int)pdfioDictGetNumber(dict, "Width");
  img->height = (int)pdfioDictGetNumber(dict, "Height");
  img->ncomps = ncomps;
  img->bpc    = (int)pdfioDictGetNumber(dict, "BitsPerComponent");

  if (img->bpc == 0)
    img->bpc = default_bpc;

  if (img->width <= 0 || img->height <= 0 || img->width > MAX_IMAGE_SIZE ||
      img->height > MAX_IMAGE_SIZE || ncomps < 1 ||
      (img->bpc != 1 && img->bpc != 2 && img->bpc != 4 && img->bpc != 8 && img->bpc != 16))
  {
    fprintf(stderr, "WARNING: Unsupported image of %dx%d pixels, %d bits per component.\n",
            img->width, img->height, img->bpc);
    return false;
  }

  img->stride = ((size_t)img->width * (size_t)ncomps * (size_t)img->bpc + 7) / 8;
  needed      = img->stride * (size_t)img->height;

  // PDFio decodes FlateDecode, including PNG/TIFF predictors
  if ((img->data = pdf_read_stream(obj, true, &size)) == NULL)
    return false;

  // Be lenient with short data: missing rows stay zero
  if (size < needed)
  {
    unsigned char *temp = realloc(img->data, needed);

    if (!temp)
    {
      free(img->data);
      img->data = NULL;
      return false;
    }

    memset(temp + size, 0, needed - size);
    img->data = temp;
  }

  return true;
}

static inline unsigned
get_sample(const unsigned char *row,
           size_t              index,
           int                 bpc)
{
  switch (bpc)
  {
    case 8 :
        return row[index];
    case 16 :
        return (unsigned)(row[2 * index] << 8 | row[2 * index + 1]);
    case 4 :
        return (row[index / 2] >> ((index & 1) ? 0 : 4)) & 15;
    case 2 :
        return (row[index / 4] >> (6 - 2 * (index & 3))) & 3;
    default :
        return (row[index / 8] >> (7 - (index & 7))) & 1;
  }
}

//
// 'get_decode()' - Get the /Decode range of each component (PDF 8.9.5.2).
//

static void
get_decode(pdfio_array_t *decode,
           int           ncomps,
           int           bpc,
           bool          indexed,
           double        dmin[4],
           double        dscale[4])
{
  double maxval = (double)((1 << bpc) - 1);

  for (int c = 0; c < ncomps && c < 4; c ++)
  {
    double low = 0.0, high = indexed ? maxval : 1.0;

    if (pdfioArrayGetSize(decode) >= (size_t)(2 * ncomps))
    {
      low  = pdfioArrayGetNumber(decode, (size_t)(2 * c));
      high = pdfioArrayGetNumber(decode, (size_t)(2 * c + 1));
    }

    dmin[c]   = low;
    dscale[c] = (high - low) / maxval;
  }
}

static inline unsigned char
to_byte(double v)
{
  return v <= 0.0 ? 0 : v >= 1.0 ? 255 : (unsigned char)(v * 255.0 + 0.5);
}

//
// 'pixel_to_rgb()' - Convert the decoded components of one pixel to RGB.
//

static void
pixel_to_rgb(const image_colorspace_t *cs,
             const double             *v,
             unsigned char            rgb[3])
{
  double	palette[4];
  image_kind_t	kind = cs->kind;

  if (kind == IMAGE_INDEXED)
  {
    int index = (int)(v[0] + 0.5);

    if (index < 0)
      index = 0;
    else if (index > cs->hival)
      index = cs->hival;

    for (int i = 0; i < cs->base_ncomps; i ++)
      palette[i] = cs->lookup[index * cs->base_ncomps + i] / 255.0;

    kind = cs->base;
    v    = palette;
  }

  switch (kind)
  {
    case IMAGE_RGB :
        rgb[0] = to_byte(v[0]);
        rgb[1] = to_byte(v[1]);
        rgb[2] = to_byte(v[2]);
        break;

    case IMAGE_CMYK :
        // Same naive conversion as the k/K operators
        rgb[0] = to_byte((1.0 - v[0]) * (1.0 - v[3]));
        rgb[1] = to_byte((1.0 - v[1]) * (1.0 - v[3]));
        rgb[2] = to_byte((1.0 - v[2]) * (1.0 - v[3]));
        break;

    default :
        rgb[0] = rgb[1] = rgb[2] = to_byte(cs->subtractive ? 1.0 - v[0] : v[0]);
        break;
  }
}

//
// 'color_surface()' - Convert samples to an RGB24 surface, or to an ARGB32
//                     surface when a color key /Mask makes pixels transparent.
//

static cairo_surface_t *
color_surface(const image_data_t       *img,
              const image_colorspace_t *cs,
              pdfio_array_t            *decode,
              pdfio_array_t            *color_key)
{
  cairo_surface_t	*surface;
  unsigned char		*pixels;
  int			stride;
  double		dmin[4], dscale[4], v[4];
  unsigned		raw[4];
  bool			keyed = pdfioArrayGetSize(color_key) >= (size_t)(2 * img->ncomps);

  surface = cairo_image_surface_create(keyed ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
                                       img->width, img->height);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
  {
    cairo_surface_destroy(surface);
    return NULL;
  }

  get_decode(decode, img->ncomps, img->bpc, cs->kind == IMAGE_INDEXED, dmin, dscale);

  cairo_surface_flush(surface);
  pixels = cairo_image_surface_get_data(surface);
  stride = cairo_image_surface_get_stride(surface);

  for (int y = 0; y < img->height; y ++)
  {
    const unsigned char	*row = img->data + (size_t)y * img->stride;
    uint32_t		*out = (uint32_t *)(pixels + (size_t)y * (size_t)stride);

    for (int x = 0; x < img->width; x ++)
    {
      unsigned char	rgb[3];
      bool		masked = keyed;

      for (int c = 0; c < img->ncomps && c < 4; c ++)
      {
        raw[c] = get_sample(row, (size_t)x * (size_t)img->ncomps + (size_t)c, img->bpc);
        v[c]   = dmin[c] + raw[c] * dscale[c];

        // Color key masking compares the raw samples (PDF 8.9.6.4)
        if (masked && (raw[c] < (unsigned)pdfioArrayGetNumber(color_key, (size_t)(2 * c)) ||
                       raw[c] > (unsigned)pdfioArrayGetNumber(color_key, (size_t)(2 * c + 1))))
          masked = false;
      }

      pixel_to_rgb(cs, v, rgb);

      // cairo pixels are native-endian, premultiplied ARGB
      out[x] = masked ? 0 : (0xFF000000u | (uint32_t)rgb[0] << 16 |
                             (uint32_t)rgb[1] << 8 | rgb[2]);
    }
  }

  cairo_surface_mark_dirty(surface);
  return surface;
}

//
// 'alpha_surface()' - Convert one-component samples to an A8 surface.
//
// A stencil (image mask or explicit /Mask) is opaque where the decoded
// sample is 0; a soft mask (/SMask) uses the decoded gray as alpha.
//

static cairo_surface_t *
alpha_surface(const image_data_t *img,
              pdfio_array_t      *decode,
              bool               stencil)
{
  cairo_surface_t	*surface;
  unsigned char		*pixels;
  int			stride;
  double		dmin[4], dscale[4];

  surface = cairo_image_surface_create(CAIRO_FORMAT_A8, img->width, img->height);
  if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
  {
    cairo_surface_destroy(surface);
    return NULL;
  }

  get_decode(decode, 1, img->bpc, false, dmin, dscale);

  cairo_surface_flush(surface);
  pixels = cairo_image_surface_get_data(surface);
  stride = cairo_image_surface_get_stride(surface);

  for (int y = 0; y < img->height; y ++)
  {
    const unsigned char	*row = img->data + (size_t)y * img->stride;
    unsigned char	*out = pixels + (size_t)y * (size_t)stride;

    for (int x = 0; x < img->width; x ++)
    {
      double v = dmin[0] + get_sample(row, (size_t)x * (size_t)img->ncomps, img->bpc) * dscale[0];

      out[x] = stencil ? (v < 0.5 ? 255 : 0) : to_byte(v);
    }
  }

  cairo_surface_mark_dirty(surface);
  return surface;
}

//
// 'load_mask()' - Load the /SMask or /Mask image of an image as alpha.
//

static cairo_surface_t *
load_mask(pdfio_obj_t *obj,
          bool        stencil)
{
  pdfio_dict_t		*dict = pdfioObjGetDict(obj);
  image_data_t		img;
  cairo_surface_t	*surface = NULL;

  if (load_samples(obj, dict, 1, stencil ? 1 : 8, &img))
    surface = alpha_surface(&img, pdf_dict_get_array(dict, "Decode"), stencil);

  free(img.data);
  return surface;
}

//
// 'pdf_draw_image()' - Draw an image XObject (Do) into the unit square of the
//                      current user space (PDF 32000-1 8.9).
//

void
pdf_draw_image(p2c_device_t *dev,
               pdfio_obj_t  *obj,
               pdfio_dict_t *resources)
{
  pdfio_dict_t		*dict = pdfioObjGetDict(obj);
  bool			interpolate = pdfioDictGetBoolean(dict, "Interpolate");
  image_colorspace_t	cs = { 0 };
  image_data_t		img = { 0 };
  cairo_surface_t	*surface = NULL, *mask = NULL;
  pdfio_obj_t		*mask_obj;
  pdfio_array_t		*color_key = NULL;
  bool			have_cs;

  if (pdfioDictGetBoolean(dict, "ImageMask"))
  {
    // Stencil mask: paint the fill color through it (PDF 8.9.6.2)
    if ((mask = load_mask(obj, true)) != NULL)
      device_fill_image_mask(dev, mask, interpolate);

    cairo_surface_destroy(mask);
    return;
  }

  have_cs = pdfioDictGetName(dict, "ColorSpace") ?
            colorspace_from_name(resources, pdfioDictGetName(dict, "ColorSpace"), &cs, 0) :
            colorspace_from_array(resources, pdf_dict_get_array(dict, "ColorSpace"), &cs, 0);

  if (!load_samples(obj, dict, have_cs ? cs.ncomps : 0, 8, &img))
    goto done;

  // Trust the components of the JPEG data when they disagree
  if (img.jpeg && (!have_cs || (cs.kind != IMAGE_INDEXED && cs.ncomps != img.ncomps)))
  {
    free(cs.lookup);
    memset(&cs, 0, sizeof(cs));
    have_cs = set_kind(&cs, img.ncomps == 4 ? IMAGE_CMYK : img.ncomps == 1 ? IMAGE_GRAY : IMAGE_RGB);
  }

  if (!have_cs || (cs.kind == IMAGE_INDEXED && img.ncomps != 1))
  {
    fprintf(stderr, "WARNING: Image color space is not supported yet.\n");
    goto done;
  }

  // Transparency: a soft mask, a stencil mask stream or a color key array
  if ((mask_obj = pdfioDictGetObj(dict, "SMask")) != NULL)
    mask = load_mask(mask_obj, false);
  else if ((color_key = pdfioDictGetArray(dict, "Mask")) == NULL &&
           (mask_obj = pdfioDictGetObj(dict, "Mask")) != NULL &&
           (color_key = pdfioObjGetArray(mask_obj)) == NULL)
    mask = load_mask(mask_obj, true);

  if ((surface = color_surface(&img, &cs, pdf_dict_get_array(dict, "Decode"), color_key)) != NULL)
    device_draw_image(dev, surface, mask, interpolate);

  done:

  cairo_surface_destroy(surface);
  cairo_surface_destroy(mask);
  free(img.data);
  free(cs.lookup);
}
