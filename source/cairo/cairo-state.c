//
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cairo-private.h"

// 
// 'cmyk_to_rgb()' - Converts CMYK color values to RGB.
// 		     PDF uses CMYK for print; Cairo requires RGB for display.
//

static void 						  // O - Void  
cmyk_to_rgb(double c, double m, double y, double k, 	// I - c, m, y, k values 
	    double *r, double *g, double *b)		// I - pointer to r, g, b values
{
  // Colour conversion formula
  *r = (1.0 - c) * (1.0 - k);
  *g = (1.0 - m) * (1.0 - k);
  *b = (1.0 - y) * (1.0 - k);
}

//
// 'device_transform()' - Modifies the Current Transformation Matrix (CTM)
//                        (corresponding to the 'cm' operator).
//

void                              // O - Void
device_transform(p2c_device_t *dev,    // I - Active Rendering Context
                 double a, double b,   // I - Matrix components
                 double c, double d,
                 double e, double f)
{
  cairo_matrix_t matrix;

  if (g_verbose)
    printf("DEBUG: Applying Transform Matrix: [%f %f %f %f %f %f]\n", a, b, c, d, e, f);

  // Initialize a cairo matrix with the PDF operands
  cairo_matrix_init(&matrix, a, b, c, d, e, f);

  // Apply the transformation to the current context
  cairo_transform(dev->cr, &matrix);
}

// --- Graphics State Management ---

//
// 'device_save_state()' - Saves the current graphics state by pushing it onto the stack.
//

void 						  // O - Void
device_save_state(p2c_device_t *dev)		// I - Active Rendering Context
{
  // Ensure we don't exceed the maximum stack depth (64)
  if (dev->gstack_ptr < (MAX_GSTATE - 1))
  {
    // Save the internal Cairo context state
    cairo_save(dev->cr);
    // Copy the current custom state structure to the next slot
    memcpy(&dev->gstack[dev->gstack_ptr + 1], &dev->gstack[dev->gstack_ptr], sizeof(graphics_state_t));
    // Move the pointer up
    dev->gstack_ptr++;

    if (g_verbose)
      printf("DEBUG: Graphics state saved. New stack level: %d\n", dev->gstack_ptr);
  }
  else
  {
    fprintf(stderr, "ERROR: Graphics state stack overflow.\n");
  }
}

//
// 'device_restore_state()' - Restores the previous graphics state (Q operator).
// 			      Pops the top state off the stack.
//

void 						  // O - Void
device_restore_state(p2c_device_t *dev)		// I - Active Rendering Context
{
  // Ensure there is a state to return to
  if (dev->gstack_ptr > 0)
  {
    // Revert the Cairo context to its previous settings
    cairo_restore(dev->cr);
    //Move the pointer down
    dev->gstack_ptr--;
    if (g_verbose)
      printf("DEBUG: Graphics state restored. New stack level: %d\n", dev->gstack_ptr);
  }
  else
  {
    fprintf(stderr, "ERROR: Graphics state stack underflow.\n");
  }
}

//
// 'device_set_line_width()' - Sets the thickness of lines for stroke 
// 			       operations (corresponding to the 'w' operator).
//

void 						  // O - Void
device_set_line_width(p2c_device_t *dev, 	// I - Active Rendering Context
		      double width)		// I - Thickness of Line
{
  if (g_verbose)
    printf("DEBUG: Setting line width to: %f\n", width);

  // Update the line width value in nternal graphics state for current stack level.
  dev->gstack[dev->gstack_ptr].line_width = width;

  // Apply the line width directly to the Cairo context so it takes effect immediately.
  cairo_set_line_width(dev->cr, width);
}

//
// 'device_set_fill_rgb()' - Sets the RGB color used for fill 
// 			   operations (corresponding to the 'rg' operator).
//

void 							  // O - Void
device_set_fill_rgb(p2c_device_t *dev,			// I - Active Rendering Context 
	            double r, double g, double b)	// I - RGB values
{
  if (g_verbose)
    printf("DEBUG: Setting fill color to RGB(%f, %f, %f)\n", r, g, b);

  // Target the graphics state at the current level of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Update the color components.
  gs->fill_rgb[0] = r;
  gs->fill_rgb[1] = g;
  gs->fill_rgb[2] = b;
  
  // Mark the colorspace as RGB.
  gs->fill_colorspace = CS_DEVICE_RGB;
}

//
// 'device_set_stroke_rgb()' - Sets the RGB color used for stroke (line) 
// 			       operations (corresponding to the 'RG' operator).
//

void 							  // O - Void
device_set_stroke_rgb(p2c_device_t *dev, 		// I - Active Rendering Context
		      double r, double g, double b)	// I - RGB values
{
  if (g_verbose)
    printf("DEBUG: Setting stroke color to RGB(%f, %f, %f)\n", r, g, b);

  // Target the graphics state at the current level of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  
  // Update the color components.
  gs->stroke_rgb[0] = r;
  gs->stroke_rgb[1] = g;
  gs->stroke_rgb[2] = b;

  // Mark the colorspace as RGB.
  gs->stroke_colorspace = CS_DEVICE_RGB;
}

//
// 'device_set_fill_gray()' - Sets the fill color using a grayscale 
// 			      value (corresponding to the 'g' operator).
//

void 						  // O - Void
device_set_fill_gray(p2c_device_t *dev, 	// I - Active Rendering Context
		     double g)			// I - Grayscale value
{
  if (g_verbose)
    printf("DEBUG: Setting fill color to Gray(%f)\n", g);

  // Target the graphics state at the current level of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  
  // For grayscale, all RGB components are set to the same value
  gs->fill_rgb[0] = g;
  gs->fill_rgb[1] = g;
  gs->fill_rgb[2] = g;

  // Update colorspace flag
  gs->fill_colorspace = CS_DEVICE_GRAY;
}

//
// 'device_set_stroke_gray()' - Sets the stroke color using a grayscale 
// 				value (corresponding to the 'G' operator).
//

void 						  // O - Void
device_set_stroke_gray(p2c_device_t *dev, 	// I - Active rendering Context
		       double g)		// I - Grayscale value
{
  if (g_verbose)
    printf("DEBUG: Setting stroke color to Gray(%f)\n", g);

  // Target the graphics state at the current level of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  
  // For grayscale, all RGB components are set to the same value
  gs->stroke_rgb[0] = g;
  gs->stroke_rgb[1] = g;
  gs->stroke_rgb[2] = g;

  // Update colorspace flag
  gs->stroke_colorspace = CS_DEVICE_GRAY;
}

//
// 'device_set_fill_cmyk()' - Sets the fill color using CMYK 
// 			      values (corresponding to the 'k' operator).
//

void 						  // O - Void	
device_set_fill_cmyk(p2c_device_t *dev, 	// I - Active Rendering Context
		     double c, double m, 	// I - c, m y, k values
		     double y, double k)
{
  if (g_verbose)
    printf("DEBUG: Setting fill color to CMYK(%f, %f, %f, %f)\n", c, m, y, k);

  // Target the graphics state at the current level of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Convert CMYK components to RGB for storage
  cmyk_to_rgb(c, m, y, k, &gs->fill_rgb[0], &gs->fill_rgb[1], &gs->fill_rgb[2]);

  // Mark colorspace as CMYK.
  gs->fill_colorspace = CS_DEVICE_CMYK;
}

//
// 'device_set_stroke_cmyk()' - Sets the stroke color using CMYK 
// 				values (corresponding to the 'K' operator)
//

void 						  // O - Void
device_set_stroke_cmyk(p2c_device_t *dev, 	// I - Active Rendering Context
		       double c, double m, 	// I - c, m y, k values
		       double y, double k)
{
  if (g_verbose)
    printf("DEBUG: Setting stroke color to CMYK(%f, %f, %f, %f)\n", c, m, y, k);

  // Target the graphics state at the current level of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  
  // Convert CMYK components to RGB for storage
  cmyk_to_rgb(c, m, y, k, &gs->stroke_rgb[0], &gs->stroke_rgb[1], &gs->stroke_rgb[2]);

  // Mark colorspace as CMYK.
  gs->stroke_colorspace = CS_DEVICE_CMYK;
}

//
// 'device_set_graphics_state()' - Applies settings from an Extended Graphics State 
// 				   dictionary (corresponding to the 'gs' operator).  
// 				   This is used for advanced parameters like 
// 				   transparency (alpha).
//

void 							  // O - Void
device_set_graphics_state(p2c_device_t *dev, 		// I - Active Rendering Context
			  pdfio_dict_t *res_dict, 	// I - resource dictionary of page
			  const char *name)		// I - Name of GS state to apply
{
  if (g_verbose)
    printf("DEBUG: Applying graphics state dictionary: /%s\n", name);

  // Retrieve the main dictionary from the page resources object
  if (!res_dict)  
    return;

  // Locate the "ExtGState" sub-dictionary within the resources.
  pdfio_obj_t *extgstate_obj = pdfioDictGetObj(res_dict, "ExtGState");
  if (!extgstate_obj) 
    return;

  // Get the dictionary representation of the ExtGState collection.
  pdfio_dict_t *extgstate_dict = pdfioObjGetDict(extgstate_obj);
  if (!extgstate_dict) 
    return;

  // Look up the specific graphics state object by the provided name.
  pdfio_obj_t *gs_obj = pdfioDictGetObj(extgstate_dict, name);
  if (!gs_obj) 
    return;

  // Get the actual dictionary containing the graphics parameters
  pdfio_dict_t *gs_dict = pdfioObjGetDict(gs_obj);
  if (!gs_dict) 
    return;

  // Access the current graphics state at the top of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  pdfio_obj_t *val_obj;

  // Check for Line Width (LW). If present, update the device's line width.
  if ((val_obj = pdfioDictGetObj(gs_dict, "LW")) != NULL)
    device_set_line_width(dev, pdfioObjGetNumber(val_obj));

  // Check for Fill Alpha (ca). Sets transparency for fill operations.
  if ((val_obj = pdfioDictGetObj(gs_dict, "ca")) != NULL)
  {
    gs->fill_alpha = pdfioObjGetNumber(val_obj);
    if (g_verbose) printf("DEBUG: Set fill alpha to %f\n", gs->fill_alpha);
  }

  // Check for Stroke Alpha (CA). Sets transparency for stroke operations.
  if ((val_obj = pdfioDictGetObj(gs_dict, "CA")) != NULL)
  {
    gs->stroke_alpha = pdfioObjGetNumber(val_obj);
    if (g_verbose) printf("DEBUG: Set stroke alpha to %f\n", gs->stroke_alpha);
  }
}
