//
// Test Program for the pdf2cairo renderer.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"  // testBegin and testEnd functions come from here
#include <dirent.h>

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
static const char *input_path = "testfiles/input/basic/";
static const char *output_path = "testfiles/renderer-output/";

/* --- ORIGINAL MANUAL TEST ARRAY (Commented Out) ---
 * Use this structure if you want to define specific flags or
 * output names for individual files.
 */
static renderer_test_t manual_tests[] = 
{
  { "test_file_1pg", "test_file_1pg.pdf", "", "T", ""},
  { "Stroked box", "01_stroked_box.pdf", "", "T", "" },
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
  { "simpleText", "simpleText.pdf", "", "T", ""}
};

// Main()
int main(void)
{
  int status = 0;
  char command[2048];

  puts(" --- Running PDF2Cairo Renderer Tests --- ");

  // Create the output directory
  testBegin("Create renderer output directory");
  if (system("mkdir -p testfiles/renderer-output") != 0)
  {
    testEndMessage(false, "Failed to create output directory.");
    return (1);
  }
  testEnd (true);

// * ____________MANUAL TEST(UNCOMMENT IT FOR TESTING INDIVIDUAL FILES_____
  int num_manual = sizeof(manual_tests) / sizeof(manual_tests[0]);
  for (int i = 0; i < num_manual; i++)
  {
    testBegin(" Manual Test: %s", manual_tests[i].description);
    
    // Construct command with specific flags (o, d, or T)
    snprintf(command, sizeof(command), "./source/tools/pdf2cairo/pdf2cairo %s %s%s",
             manual_tests[i].input_args, input_path, manual_tests[i].input_file);

    if (strcmp(manual_tests[i].output_option, "o") == 0) {
      strncat(command, " -o ", sizeof(command) - strlen(command) - 1);
      strncat(command, output_path, sizeof(command) - strlen(command) - 1);
      strncat(command, manual_tests[i].output_filename, sizeof(command) - strlen(command) - 1);
    } else {
      strncat(command, " -T", sizeof(command) - strlen(command) - 1);
    }

    if (system(command) != 0) status = 1, testEnd(false);
    else testEnd(true);
  }
 /* 
  // Open directory containing the PDF files
  DIR *dir = opendir(input_path);
  struct dirent *entry;
  if (dir)
  {
    while ((entry = readdir(dir)) != NULL) 
    {
      if (strstr(entry->d_name, ".pdf")) 
      {
        testBegin("Test: %s", entry->d_name);
        snprintf(command, sizeof(command), "./source/tools/pdf2cairo/pdf2cairo -T %s%s",
                 input_path, entry->d_name);

        if (system(command) == 0) 
	{
	  testEnd(true);
	}
        else 
	{
          testEnd(false);
	  status = 1; 
	}
      }
    }
    closedir(dir);
  }
  else
  {
    fprintf(stderr, "Could not open directory: %s\n", input_path);
    return 1;
  }
  */
  
  puts(" --- Renderer Test finished. Check files in the testfiles/renderer-output/ --- ");
  return (status);
}
