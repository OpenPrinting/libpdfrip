#include "cairo_device_internal.h"

// --- Internal Helper Functions ---

//
// '_apply_fill_color()' - Sets the current cairo source to the device's fill color 
// 			   and alpha.
// 			   This ensures the "paint" matches the current PDF graphics 
// 			   state for filling.
//

static void 					  // O - Void
_apply_fill_color(p2c_device_t *dev)		// I - Active Rendering Context
{
  // Target the graphics state at the top of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Apply RGB and alpha transparency to the Cairo context.
  cairo_set_source_rgba(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2], gs->fill_alpha);
}

//
// '_apply_stroke_color()' - Sets the current cairo source to the device's stroke color 
// 			     and alpha. 
// 			     Used for drawing lines and outlines.
//

static void 					  // O - Void
_apply_stroke_color(p2c_device_t *dev)		// I - Active Rendering Context
{
  // Target the graphics state at the top of the stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Sets Cairo source to the current stroke color settings.
  cairo_set_source_rgba(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2], gs->stroke_alpha);
}

// --- Path Construction ---

//
// 'device_move_to()' - Starts a new sub-path at the specified (x, y) coordinates.
//

void 						  // O - Void				
device_move_to(p2c_device_t *dev,		// I - Active Rendering Context
	       double x, double y)		// I - X and Y coordinates
{
  if (g_verbose) 
    printf("DEBUG: Path Move To (%f, %f)\n", x, y);

  // Updates Cairo's current point.
  cairo_move_to(dev->cr, x, y);
}

//
// 'device_line_to()' - Adds a straight line segment from the current point to (x, y).
//

void 						  // O - Void
device_line_to(p2c_device_t *dev, 		// I - Active Rendering Context
	       double x, double y)		// I - X and Y coordinates
{
  if (g_verbose) 
    printf("DEBUG: Path Line To (%f, %f)\n", x, y);

  // Draws a line in the current path.
  cairo_line_to(dev->cr, x, y);
}

//
// 'device_curve_to()' - Adds a cubic Bézier curve to the current path.
//

void 						  // O - Void
device_curve_to(p2c_device_t *dev, 		// I - Active Rendering Context
		double x1, double y1, 		// I - Control point 1 coordinates
		double x2, double y2, 		// I - Control point 2 coordinates
		double x3, double y3)		// I - End Point coordinates
{
  if (g_verbose) 
    printf("DEBUG: Path Curve To (%f,%f %f,%f %f,%f)\n", x1, y1, x2, y2, x3, y3);

  // Adds a curve using two control points (x1,y1), (x2,y2) and an endpoint (x3,y3).
  cairo_curve_to(dev->cr, x1, y1, x2, y2, x3, y3);
}

//
// 'device_rectangle()' - Adds a closed rectangle sub-path.
//

void 						  // O - Void
device_rectangle(p2c_device_t *dev, 		// I - Active Rendering Context
		 double x, double y, 		// I - Coordinate of lower left coordinates
		 double w, double h)		// I - Width and Height of Rectangle
{
  if (g_verbose) 
    printf("DEBUG: Path Rectangle (%f,%f size %f x %f)\n", x, y, w, h);

  // Defines a rectangle at (x,y) with width w and height h.
  cairo_rectangle(dev->cr, x, y, w, h);
}

//
// 'device_close_path()' - Closes the current sub-path by drawing a line back to the start.
//

void 						  // O - Void
device_close_path(p2c_device_t *dev)		// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Path Close\n");

  // Draw a line back to start
  cairo_close_path(dev->cr);
}

// --- Path Painting ---

//
// 'device_stroke()' - Strokes the current path with the current line 
// 		       settings (S operator).
//

void 						  // O - Void
device_stroke(p2c_device_t *dev)		// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Paint Stroke\n");

  // Prepare Cairo with the current stroke color and transparency.
  _apply_stroke_color(dev);
  
  // Perform the actual drawing operation.
  cairo_stroke(dev->cr);
}

//
// 'device_fill()' - Fills the current path with the current fill settings (f operator).
//

void 						  // O - Void
device_fill(p2c_device_t *dev)			// I - Active Rendering Context	
{
  if (g_verbose) 
    printf("DEBUG: Paint Fill\n");

  // Prepare Cairo with the current fill color and transparency.
  _apply_fill_color(dev);

  // Fill the interior of the path.
  cairo_fill(dev->cr);
}

//
// 'device_fill_preserve()' - Fills the path but preserves it for a subsequent 
// 			      operation (e.g., stroking). 
// 			      This is used for operators like 'B' (Fill and Stroke).
//

void 						  // O - Void
device_fill_preserve(p2c_device_t *dev)		// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Paint Fill Preserve\n");

  // Prepare Cairo with the current fill color and transparency.
  _apply_fill_color(dev);

  // Fill the interior but DO NOT clear the path from Cairo's memory.
  cairo_fill_preserve(dev->cr);
}

//
// 'device_fill_even_odd()' - Fills the interior using the Even-Odd rule (f* operator).
//

void 						  // O - Void
device_fill_even_odd(p2c_device_t *dev)		// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Paint Fill (Even/Odd Rule)\n");

  // Prepare Cairo with the current fill color and transparency.
  _apply_fill_color(dev);
  
  // Temporarily change the fill rule.
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_fill(dev->cr);

  // Reset to the default non-zero winding rule.
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING); // Reset to default
}

//
// 'device_fill_preserve_even_odd()' - Fills using the Even-Odd rule while 
// 				       preserving the path for a later 
// 				       stroke (B* operator).
//

void 							  // O - Void		
device_fill_preserve_even_odd(p2c_device_t *dev)	// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Paint Fill Preserve (Even/Odd Rule)\n");

  // Prepare Cairo with the current fill color and transparency.
  _apply_fill_color(dev);

  // Set rule to Even-Odd.
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);

  // Fill the interior and keep the path geometry.
  cairo_fill_preserve(dev->cr);
 
  // Reset the fill rule to default.
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING); 
}

// --- Clipping Paths ---

//
// 'device_clip()' - Uses the current path to restrict all future 
// 		     drawing operations (W operator).
//

void 						  // O - Void
device_clip(p2c_device_t *dev)			// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Set Clip Path\n");

  // Ensure the Non-Zero rule is used for the clip.
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_WINDING);

  // Intersect the current clipping area with the current path.
  cairo_clip(dev->cr);

  // Clear the current path to prevent it from being drawn as a shape.
  cairo_new_path(dev->cr);
}

//
// 'device_clip_even_odd()' - Defines a clipping area using the Even-Odd 
// 			      rule (W* operator).
//

void 						  // O - Void
device_clip_even_odd(p2c_device_t *dev)		// I - Active Rendering Context
{
  if (g_verbose) 
    printf("DEBUG: Set Clip Path (Even/Odd Rule)\n");

  // Set the Even-Odd rule for the clip.
  cairo_set_fill_rule(dev->cr, CAIRO_FILL_RULE_EVEN_ODD);

  // Apply the clip.
  cairo_clip(dev->cr);

  // Clear the path.
  cairo_new_path(dev->cr);
}

//
// 'device_get_current_point()' - Retrieves the current point from the Cairo path.
//                                Used for 'v' operator.
//

void                              			  // O - Void
device_get_current_point(p2c_device_t *dev, 		// I - Active Rendering Context
                         double *x, double *y) 		// O - Current X and Y
{
  if (cairo_has_current_point(dev->cr))
  {
    cairo_get_current_point(dev->cr, x, y);
  }
  else
  {
    *x = 0.0;
    *y = 0.0;
  }
}
