//
// Test Program for the pdf2cairo renderer.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"  // testBegin and testEnd functions come from here

// Structure to hold a single renderer test case
typedef struct
{
  const char *description; // A short description of the test
  const char *input_file;  // Input PDF file name
  const char *input_args;  // Command-line arguments for the input file
  const char *output_option; // Output option: "o", "d", "T", or NULL
  const char *output_filename; // Filename to be used with "-o", or NULL
} renderer_test_t;

// Common paths for test files
static const char *input_path = "testfiles/renderer/";
static const char *output_path = "testfiles/renderer-output/";

// Array of renderer test cases
static renderer_test_t tests[] = 
{
  { "Stroked box", "01_stroked_box.pdf", "", "o", "01.png" },
  { "Filled RGB box", "02_filled_box_rgb.pdf", "", "T", ""},
  { "Nested Box", "03_nested_state.pdf", "", "T", ""},
  { "Cubic_Bezier_Curve", "04_Cubic_Bezier_Curve.pdf", "", "T", ""},
  { "Curves" , "05_Curves.pdf", "", "T", ""},
  { "Fill and Stroke", "06_fill_and_stroke.pdf", "", "T", ""},
  { "Shape with hole", "07_shape_with_holes.pdf", "", "T", ""},
  { "TestFilledBanners", "TestFilledBanners.pdf", "", "T", ""},
  { "TestFilledBasicShapesPart1", "TestFilledBasicShapesPart1.pdf", "", "T", ""},
  { "TestFilledBasicShapesPart2", "TestFilledBasicShapesPart2.pdf", "", "T", ""},
  { "TestFilledBlockArrows", "TestFilledBlockArrows.pdf", "", "T", ""},
  { "TestFilledEquationShapes", "TestFilledEquationShapes.pdf", "", "T", ""},
  { "TestFilledFlowChart", "TestFilledFlowChart.pdf", "", "T", ""},
  { "TestFilledRectangles", "TestFilledRectangles.pdf", "", "T", ""},
  { "TestFilledStars", "TestFilledStars.pdf", "", "T", ""},
  { "TestStrokedBanners", "TestStrokedBanners.pdf", "" , "T", ""},
  { "TestStrokedBasicShapesPart1", "TestStrokedBasicShapesPart1.pdf", "", "T", ""},
  { "TestStrokedBasicShapesPart2", "TestStrokedBasicShapesPart2.pdf", "", "T", ""},
  { "TestStrokedBlockArrows", "TestStrokedBlockArrows.pdf", "", "T", ""},
  { "TestStrokedEquationShapes", "TestStrokedEquationShapes.pdf", "", "T", ""},
  { "TestStrokedFlowChart", "TestStrokedFlowChart.pdf", "", "T", ""},
  { "TestStrokedRectangles", "TestStrokedRectangles.pdf", "", "T", ""},
  { "TestStrokedStars", "TestStrokedStars.pdf", "", "T", ""},
  { "TestTables", "TestTables.pdf", "", "T", ""},
  { "test_file_1pg", "test_file_1pg.pdf", "", "T", ""},
  { "simpleText", "simpleText.pdf", "-v", "T", ""}
};

// Main()
int main(void)
{
  int i;
  int status = 0;
  int num_tests = sizeof(tests) / sizeof(tests[0]);

  puts(" --- Running PDF2Cairo Renderer Tests --- ");

  // Create the output directory
  testBegin("Create renderer output directory");
  if (system("mkdir -p testfiles/renderer-output") != 0)
  {
    testEndMessage(false, "Failed to create output directory.");
    return (1);
  }
  testEnd (true);
  
  // Loop through all of the test cases
  for (i = 0; i < num_tests; i++)
  {
    char command[2048]; // Command to execute

    // Format the command that will be run
    // *** THIS LINE IS UPDATED ***
    // It now points to the new path in the same directory.
    snprintf(command, sizeof(command), "./source/tools/pdf2cairo/pdf2cairo %s%s%s%s",
        tests[i].input_args,
        (tests[i].input_args[0]!='\0') ? " ": "",
        input_path,
        tests[i].input_file);

    // Append output argumetns based on the test case specification
    if (tests[i].output_option)
    {
      if (strcmp(tests[i].output_option, "o") == 0 && tests[i].output_filename)
      {
        // -o <path/filename>
        strncat(command, " -o ", sizeof(command) - strlen(command) - 1);
        strncat(command, output_path, sizeof(command) - strlen(command) - 1);
        strncat(command, tests[i].output_filename, sizeof(command) - strlen(command) - 1);
      }
      else if (strcmp(tests[i].output_option, "d") == 0)
      {
        // -d <path>
        strncat(command, " -d ", sizeof(command) - strlen(command) - 1);
        strncat(command, output_path, sizeof(command) - strlen(command) - 1);
      }
      else if (strcmp(tests[i].output_option, "T") == 0)
      {
        // - T
        strncat(command, " -T ", sizeof(command) - strlen(command) - 1);
      }
    }

    // Run the test
    testBegin(" Test: %s", tests[i].description);

    if (system(command) != 0)
    {
      // Command failed; 
      testEnd(false);
      status = 1;
    }
    else 
    {
      // Command succeeded
      testEnd(true);
    }
  }

  puts(" --- Renderer Test finished. Check files in the testfiles/renderer-output/ --- ");

  return (status);
}
