#include "cairo_device_internal.h"

/**
 * WinAnsiEncoding maps character codes to glyph names.
 * See Appendix D in the PDF 1.7 Specification.
 * NULL entries represent unassigned character codes.
 */
static const char *WIN_ANSI_ENCODING[256] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "/space", "/exclam", "/quotedbl", "/numbersign", "/dollar", "/percent", "/ampersand", "/quotesingle",
    "/parenleft", "/parenright", "/asterisk", "/plus", "/comma", "/hyphen", "/period", "/slash",
    "/zero", "/one", "/two", "/three", "/four", "/five", "/six", "/seven", "/eight", "/nine",
    "/colon", "/semicolon", "/less", "/equal", "/greater", "/question", "/at", "/A", "/B", "/C",
    "/D", "/E", "/F", "/G", "/H", "/I", "/J", "/K", "/L", "/M", "/N", "/O", "/P", "/Q", "/R", "/S",
    "/T", "/U", "/V", "/W", "/X", "/Y", "/Z", "/bracketleft", "/backslash", "/bracketright",
    "/asciicircum", "/underscore", "/grave", "/a", "/b", "/c", "/d", "/e", "/f", "/g", "/h",
    "/i", "/j", "/k", "/l", "/m", "/n", "/o", "/p", "/q", "/r", "/s", "/t", "/u", "/v", "/w",
    "/x", "/y", "/z", "/braceleft", "/bar", "/braceright", "/asciitilde", NULL, "/Euro",
    "/bullet", "/quotesinglbase", "/florin", "/quotedblbase", "/ellipsis", "/dagger", "/daggerdbl",
    "/circumflex", "/perthousand", "/Scaron", "/guilsinglleft", "/OE", "/bullet", "/Zcaron",
    "/bullet", "/bullet", "/quoteleft", "/quoteright", "/quotedblleft", "/quotedblright", "/bullet",
    "/endash", "/emdash", "/tilde", "/trademark", "/scaron", "/guilsinglright", "/oe", "/bullet",
    "/zcaron", "/Ydieresis", "/space", "/exclamdown", "/cent", "/sterling", "/currency", "/yen",
    "/brokenbar", "/section", "/dieresis", "/copyright", "/ordfeminine", "/guillemotleft",
    "/logicalnot", "/hyphen", "/registered", "/macron", "/degree", "/plusminus", "/twosuperior",
    "/threesuperior", "/acute", "/mu", "/paragraph", "/periodcentered", "/cedilla", "/onesuperior",
    "/ordmasculine", "/guillemotright", "/onequarter", "/onehalf", "/threequarters", "/questiondown",
    "/Agrave", "/Aacute", "/Acircumflex", "/Atilde", "/Adieresis", "/Aring", "/AE", "/Ccedilla",
    "/Egrave", "/Eacute", "/Ecircumflex", "/Edieresis", "/Igrave", "/Iacute", "/Icircumflex",
    "/Idieresis", "/Eth", "/Ntilde", "/Ograve", "/Oacute", "/Ocircumflex", "/Otilde", "/Odieresis",
    "/multiply", "/Oslash", "/Ugrave", "/Uacute", "/Ucircumflex", "/Udieresis", "/Yacute",
    "/Thorn", "/germandbls", "/agrave", "/aacute", "/acircumflex", "/atilde", "/adieresis",
    "/aring", "/ae", "/ccedilla", "/egrave", "/eacute", "/ecircumflex", "/edieresis", "/igrave",
    "/iacute", "/icircumflex", "/idieresis", "/eth", "/ntilde", "/ograve", "/oacute", "/ocircumflex",
    "/otilde", "/odieresis", "/divide", "/oslash", "/ugrave", "/uacute", "/ucircumflex",
    "/udieresis", "/yacute", "/thorn", "/ydieresis"
};

// --- Text State ---
void device_begin_text(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Begin Text Object\n");
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_init_identity(&gs->text_matrix);
  cairo_matrix_init_identity(&gs->text_line_matrix);
}

void device_end_text(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: End Text Object\n");
}

void device_set_text_leading(p2c_device_t *dev, double leading)
{
  if (g_verbose)
    printf("DEBUG: Set Text Leading to %f\n", leading);
  dev->gstack[dev->gstack_ptr].text_leading = leading;
}

void device_move_text_cursor(p2c_device_t *dev, double tx, double ty)
{
  if (g_verbose)
    printf("DEBUG: Move Text Cursor by (%f, %f)\n", tx, ty);

  cairo_matrix_t trans_matrix;
  cairo_matrix_init_translate(&trans_matrix, tx, ty);

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_multiply(&gs->text_line_matrix, &trans_matrix, &gs->text_line_matrix);

  memcpy(&gs->text_matrix, &gs->text_line_matrix, sizeof(cairo_matrix_t));
}

void device_next_line(p2c_device_t *dev)
{
  if (g_verbose)
    printf("DEBUG: Move to Next Line\n");
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  device_move_text_cursor(dev, 0, -gs->text_leading);
}

void device_set_text_matrix(p2c_device_t *dev, double a, double b, double c, double d, double e, double f)
{
  if (g_verbose)
    printf("DEBUG: Set Text Matrix to [%f %f %f %f %f %f]\n", a, b, c, d, e, f);

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  cairo_matrix_init(&gs->text_matrix, a, b, c, d, e, f);
  memcpy(&gs->text_line_matrix, &gs->text_matrix, sizeof(cairo_matrix_t));
}

/**
 * Load a default system font as fallback
 */
static bool load_default_font(p2c_device_t *dev, double font_size)
{
    graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    
    // Try common system font paths
    const char *default_fonts[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        NULL
    };
    
    for (int i = 0; default_fonts[i] != NULL; i++)
    {
        FT_Error ft_error = FT_New_Face(dev->ft_library, default_fonts[i], 0, &gs->ft_face);
        if (ft_error == 0)
        {
            fprintf(stderr, "DEBUG: ✓ Loaded default font: %s\n", default_fonts[i]);
            
            // Set the font size
            FT_Set_Char_Size(gs->ft_face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
            
            // Set encoding map
            gs->encoding_map = NULL;
            
            return true;
        }
    }
    
    fprintf(stderr, "DEBUG: ✗ Could not load any default font\n");
    return false;
}

/**
 * Load embedded font from PDF
 */
static bool load_embedded_font(p2c_device_t *dev, pdfio_obj_t *font_obj, double font_size)
{
    graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    pdfio_dict_t *font_dict = pdfioObjGetDict(font_obj);
    
    if (!font_dict)
    {
        fprintf(stderr, "DEBUG: ✗ Font object has no dictionary\n");
        return false;
    }
    
    // Get FontDescriptor
    pdfio_obj_t *font_descriptor = pdfioDictGetObj(font_dict, "FontDescriptor");
    if (!font_descriptor)
    {
        fprintf(stderr, "DEBUG: No FontDescriptor found\n");
        return false;
    }
    
    pdfio_dict_t *descriptor_dict = pdfioObjGetDict(font_descriptor);
    if (!descriptor_dict)
    {
        fprintf(stderr, "DEBUG: ✗ FontDescriptor has no dictionary\n");
        return false;
    }
    
    // Check for embedded font stream (FontFile2 for TrueType, FontFile for Type1)
    pdfio_obj_t *font_file = pdfioDictGetObj(descriptor_dict, "FontFile2");
    if (!font_file)
    {
        font_file = pdfioDictGetObj(descriptor_dict, "FontFile");
    }
    if (!font_file)
    {
        font_file = pdfioDictGetObj(descriptor_dict, "FontFile3");
    }
    
    if (!font_file)
    {
        fprintf(stderr, "DEBUG: No embedded font stream found\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: ✓ Found embedded font (obj %zu)\n", pdfioObjGetNumber(font_file));
    
    // Get the stream dictionary to find the length
    pdfio_dict_t *stream_dict = pdfioObjGetDict(font_file);
    if (!stream_dict)
    {
        fprintf(stderr, "DEBUG: ✗ Font stream has no dictionary\n");
        return false;
    }
    
    size_t font_data_length = (size_t)pdfioDictGetNumber(stream_dict, "Length");
    if (font_data_length == 0)
    {
        fprintf(stderr, "DEBUG: ✗ Font stream length is 0\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Font data length: %zu bytes\n", font_data_length);

    // Allocate memory for font data with extra buffer
    unsigned char *font_data = malloc(font_data_length + 1024);
    if (!font_data)
    {
      fprintf(stderr, "DEBUG: ✗ Failed to allocate memory for font data\n");
      return false;
    }

    // Open and read the font stream
    pdfio_stream_t *font_stream = pdfioObjOpenStream(font_file, true);
    if (!font_stream)
    {
      fprintf(stderr, "DEBUG: ✗ Failed to open font stream\n");
      free(font_data);
      return false;
    }

    // Read all font data into memory
    ssize_t bytes_read = pdfioStreamRead(font_stream, font_data, font_data_length);

    // CRITICAL: Close the stream IMMEDIATELY to free the object
    pdfioStreamClose(font_stream);
    fprintf(stderr, "DEBUG: ✓ Font stream closed successfully\n");

    if (bytes_read <= 0)
    {
      fprintf(stderr, "DEBUG: ✗ Failed to read font data (read %zd bytes)\n", bytes_read);
      free(font_data);
      return false;
    }

    fprintf(stderr, "DEBUG: ✓ Read %zd bytes of font data\n", bytes_read);
    
    // Load font into FreeType
    FT_Error ft_error = FT_New_Memory_Face(
        dev->ft_library,
        font_data,
        bytes_read,
        0,  // face_index
        &gs->ft_face
    );
    
    if (ft_error != 0)
    {
        fprintf(stderr, "DEBUG: ✗ FreeType error loading font: %d\n", ft_error);
        free(font_data);
        return false;
    }
    
    fprintf(stderr, "DEBUG: ✓ Font loaded into FreeType successfully!\n");
    
    // Set the font size
    FT_Set_Char_Size(gs->ft_face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
    
    // Set encoding map
    gs->encoding_map = WIN_ANSI_ENCODING;
    
    // Store font data pointer so we can free it later
    // You'll need to add a field to graphics_state_t: unsigned char *font_data;
    gs->font_data = font_data;
    
    fprintf(stderr, "DEBUG: Font size set to %.2f\n", font_size);
    return true;
}

/**
 * Find font object in PDF resources
 */
static pdfio_obj_t* find_font_object(p2c_device_t *dev, const char *font_name)
{
    pdfio_obj_t *font_obj = NULL;
    
    // Primary lookup: Check dev->font_dict first
    if (dev->font_dict)
    {
        fprintf(stderr, "DEBUG: Trying primary lookup in dev->font_dict...\n");
        font_obj = pdfioDictGetObj(dev->font_dict, font_name);
        if (font_obj)
        {
            fprintf(stderr, "DEBUG: ✓ Font '%s' found in dev->font_dict!\n", font_name);
            return font_obj;
        }
        fprintf(stderr, "DEBUG: ✗ Font '%s' NOT found in dev->font_dict\n", font_name);
    }
    else
    {
        fprintf(stderr, "DEBUG: dev->font_dict is NULL, skipping primary lookup\n");
    }
    
    // Fallback: Walk up the page tree
    fprintf(stderr, "DEBUG: Starting fallback - walking page tree...\n");
    pdfio_obj_t *current_page = dev->page;
    int level = 0;
    
    while (current_page && !font_obj && level < 10)
    {
        fprintf(stderr, "DEBUG: [Level %d] Checking page object %zu...\n", 
                level, pdfioObjGetNumber(current_page));
        
        pdfio_dict_t *page_dict = pdfioObjGetDict(current_page);
        if (!page_dict)
        {
            fprintf(stderr, "DEBUG: [Level %d] Page dict is NULL\n", level);
            break;
        }
        
        // Get Resources dictionary
        pdfio_dict_t *resources_dict = NULL;
        pdfio_obj_t *resources_obj = pdfioDictGetObj(page_dict, "Resources");
        
        if (resources_obj)
        {
            fprintf(stderr, "DEBUG: [Level %d] Resources found as indirect object %zu\n", 
                    level, pdfioObjGetNumber(resources_obj));
            resources_dict = pdfioObjGetDict(resources_obj);
        }
        else
        {
            resources_dict = pdfioDictGetDict(page_dict, "Resources");
            if (resources_dict)
            {
                fprintf(stderr, "DEBUG: [Level %d] Resources found as direct dictionary\n", level);
            }
        }
        
        if (resources_dict)
        {
            // Get Font dictionary
            pdfio_dict_t *font_dict = NULL;
            pdfio_obj_t *font_dict_obj = pdfioDictGetObj(resources_dict, "Font");
            
            if (font_dict_obj)
            {
                fprintf(stderr, "DEBUG: [Level %d] Font dict found as indirect object %zu\n", 
                        level, pdfioObjGetNumber(font_dict_obj));
                font_dict = pdfioObjGetDict(font_dict_obj);
            }
            else
            {
                font_dict = pdfioDictGetDict(resources_dict, "Font");
                if (font_dict)
                {
                    fprintf(stderr, "DEBUG: [Level %d] Font dict found as direct dictionary\n", level);
                }
            }
            
            if (font_dict)
            {
                font_obj = pdfioDictGetObj(font_dict, font_name);
                if (font_obj)
                {
                    fprintf(stderr, "DEBUG: [Level %d] ✓ Font '%s' found (obj %zu)!\n", 
                            level, font_name, pdfioObjGetNumber(font_obj));
                    
                    const char *subtype = pdfioObjGetSubtype(font_obj);
                    const char *basefont = pdfioDictGetName(pdfioObjGetDict(font_obj), "BaseFont");
                    fprintf(stderr, "DEBUG: Font Subtype: %s\n", subtype ? subtype : "(null)");
                    fprintf(stderr, "DEBUG: Font BaseFont: %s\n", basefont ? basefont : "(null)");
                    
                    return font_obj;
                }
            }
        }
        
        // Move to parent
        current_page = pdfioDictGetObj(page_dict, "Parent");
        level++;
    }
    
    return NULL;
}

void device_set_font(p2c_device_t *dev, const char *font_name, double font_size)
{
    graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    
    fprintf(stderr, "\n=== DEBUG: device_set_font START ===\n");
    fprintf(stderr, "DEBUG: Looking for font: '%s' with size: %.2f\n", font_name, font_size);
    
    // Clean up previous font if exists
    if (gs->ft_face)
    {
        FT_Done_Face(gs->ft_face);
        gs->ft_face = NULL;
    }
    if (gs->font_data)
    {
        free(gs->font_data);
        gs->font_data = NULL;
    }
    
    // Set font size
    gs->font_size = font_size;
    
    // Find the font object
    pdfio_obj_t *font_obj = find_font_object(dev, font_name);
    
    bool font_loaded = false;
    
    // Try to load embedded font
    if (font_obj)
    {
        font_loaded = load_embedded_font(dev, font_obj, font_size);
    }
    
    // If embedded font failed, try default system font
    if (!font_loaded)
    {
        fprintf(stderr, "DEBUG: Embedded font not available, trying default system font...\n");
        font_loaded = load_default_font(dev, font_size);
    }
    
    if (!font_loaded)
    {
        fprintf(stderr, "DEBUG: ✗ Failed to load any font - text will not render\n");
    }
    
    fprintf(stderr, "=== DEBUG: device_set_font END ===\n\n");
}

static double
_device_show_text_internal(p2c_device_t *dev, const char *str)
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  FT_Face ft_face = gs->ft_face;
  const char **encoding_map = gs->encoding_map;
  size_t len = strlen(str);
  
  fprintf(stderr, "DEBUG: _device_show_text_internal called with text: \"%s\"\n", str);
  fprintf(stderr, "DEBUG: ft_face = %p\n", (void*)ft_face);
  
  if (!ft_face)
  {
    fprintf(stderr, "DEBUG: Skipping text draw - ft_face is NULL (font not loaded)\n");
    return 0.0;
  }

  cairo_glyph_t *glyphs = malloc(len * sizeof(cairo_glyph_t));
  if (!glyphs) return 0.0;

  int num_glyphs = 0;
  double total_advance_x = 0.0;

  for (size_t i = 0; i < len; i++)
  {
    unsigned char char_code = (unsigned char)str[i];
    unsigned int glyph_index = 0;
    
    // Try direct Unicode mapping first (works for most TrueType fonts)
    glyph_index = FT_Get_Char_Index(ft_face, char_code);
    
    // If that fails and we have an encoding map, try glyph name lookup
    if (glyph_index == 0 && encoding_map && encoding_map[char_code])
    {
      const char *glyph_name = encoding_map[char_code];
      glyph_index = FT_Get_Name_Index(ft_face, (char *)glyph_name);
      
      if (g_verbose && glyph_index == 0)
        fprintf(stderr, "DEBUG: Glyph not found for char_code=%d, glyph_name=%s\n", 
                char_code, glyph_name);
    }
    
    // Fallback: use char_code directly as glyph index
    if (glyph_index == 0)
    {
      glyph_index = char_code;
    }

    glyphs[num_glyphs].index = glyph_index;
    glyphs[num_glyphs].x = total_advance_x;
    glyphs[num_glyphs].y = 0.0;  // Y position is 0 in glyph space
    num_glyphs++;

    if (FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT) == 0)
    {
      total_advance_x += (double)ft_face->glyph->advance.x / 64.0;
    }
  }

  cairo_save(dev->cr);
  
  // Apply the text matrix transformation
  // The text matrix contains the position and any rotation/scaling
  cairo_matrix_t text_transform;
  cairo_matrix_init(&text_transform,
                    gs->text_matrix.xx,
                    gs->text_matrix.yx,
                    gs->text_matrix.xy,
                    gs->text_matrix.yy,
                    gs->text_matrix.x0,
                    gs->text_matrix.y0);
  cairo_transform(dev->cr, &text_transform);
  
  // Create a Cairo font face from FreeType face
  cairo_font_face_t *cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
  cairo_set_font_face(dev->cr, cairo_face);
  cairo_set_font_size(dev->cr, gs->font_size);
  
  // Set the fill color
  cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);
  
  // Render glyphs based on text rendering mode
  switch (gs->text_rendering_mode)
  {
    case 0:  // Fill
      cairo_show_glyphs(dev->cr, glyphs, num_glyphs);
      break;
    case 1:  // Stroke
      cairo_set_source_rgb(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2]);
      cairo_glyph_path(dev->cr, glyphs, num_glyphs);
      cairo_stroke(dev->cr);
      break;
    case 2:  // Fill then stroke
      cairo_glyph_path(dev->cr, glyphs, num_glyphs);
      cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);
      cairo_fill_preserve(dev->cr);
      cairo_set_source_rgb(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2]);
      cairo_stroke(dev->cr);
      break;
    case 3:  // Invisible
      // Do nothing
      break;
    default:  // Default to fill
      cairo_show_glyphs(dev->cr, glyphs, num_glyphs);
      break;
  }
  
  cairo_font_face_destroy(cairo_face);
  cairo_restore(dev->cr);
  free(glyphs);

  // Return the advance width in text space units
  return total_advance_x * (gs->font_size / (double)ft_face->units_per_EM);
}

void device_show_text(p2c_device_t *dev, const char *str)
{
  if (g_verbose)
    printf("DEBUG: Show Text (Tj): \"%s\"\n", str);

  double advance = _device_show_text_internal(dev, str);
  cairo_matrix_translate(&dev->gstack[dev->gstack_ptr].text_matrix, advance, 0);
}

void device_show_text_kerning(p2c_device_t *dev, operand_t *operands, int num_operands)
{
  if (g_verbose)
    printf("DEBUG: Show Text with Kerning (TJ)\n");

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  for (int i = 0; i < num_operands; i++)
  {
    if (operands[i].type == OP_TYPE_STRING)
    {
      double advance = _device_show_text_internal(dev, operands[i].value.string);
      cairo_matrix_translate(&gs->text_matrix, advance, 0);
    }
    else if (operands[i].type == OP_TYPE_NUMBER)
    {
      double adjustment = -operands[i].value.number / 1000.0 * gs->font_size;
      if (g_verbose)
        printf("DEBUG: TJ applying kerning adjustment: %f units\n", adjustment);
      cairo_matrix_translate(&gs->text_matrix, adjustment, 0);
    }
  }
}

void device_set_text_rendering_mode(p2c_device_t *dev, int mode)
{
  if (g_verbose)
    printf("DEBUG: Set Text Rendering Mode to %d\n", mode);

  if (mode >= 0 && mode <= 7)
  {
    dev->gstack[dev->gstack_ptr].text_rendering_mode = mode;
  }
}
