#include "pdfio.h"
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <time.h>   
#include <unistd.h>

#include "analyzer.h"
#include "cairo_device.h"
#include "interpreter.h"

int g_verbose = 0;

//
// 'print_usage()' - Function to show command-line help.
//
static void
print_usage(const char *prog_name) // I - Program name
{
  fprintf(stderr, "Usage: %s [options] input.pdf\n", prog_name);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  --analyze              Analyze the PDF content stream instead of rendering.\n");
  fprintf(stderr, "  --help                 Display this help message and exit.\n");
  fprintf(stderr, "  -o <output.png>        Specify the output PNG filename (render mode).\n");
  fprintf(stderr, "  -p <pagenum>           Specify the page number to process (default: 1).\n");
  fprintf(stderr, "  -r <dpi>               Specify the resolution in DPI (default: 72).\n");
  fprintf(stderr, "  -t                     Generate a temporary filename (e.g., 'inputResult123.png').\n");
  fprintf(stderr, "                         Must be used with the -d option.\n");
  fprintf(stderr, "  -d <directory>         Specify the output directory when using -t.\n");
  fprintf(stderr, "  -T                     Generate a temporary filename in 'testfiles/renderer-output/'.\n");
  fprintf(stderr, "  -v                     Enable verbose debugging output.\n"); 
}

//
// 'main()' - Main entry for the pdf2cairo_tool
//

int            // O - Exit status
main(int argc, // I - Number of command-line args
     char *argv[]) // I - Command-line arguments
{
  // --- Local Variable ---
  pdfio_file_t *pdf;
  pdfio_obj_t *page;
  pdfio_rect_t mediabox;
  int status = 0;
  int opt;

  // Command-line options
  char *input_filename = NULL;
  char *output_filename = NULL;
  int pagenum = 1;
  int dpi = 72;
  int analyze_mode = 0;

  // Variables for new flags
  char *output_dir = NULL;
  int t_flag = 0;
  int T_flag = 0;
  char temp_output_filename[1024]; // Buffer for generated filename

  // Seed the random number generator
  srand(time(NULL));

  // --- Argument Parsing ---

  // First, check for --help and --analyze flags manually
  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      print_usage(argv[0]);
      return (0);
    }
    else if (!strcmp(argv[i], "--analyze"))
    {
      analyze_mode = 1;
      // Shift remaining arguments down
      for (int j = i; j < argc - 1; j++)
      {
        argv[j] = argv[j + 1];
      }
      argc--;
      break;
    }
  }

  while ((opt = getopt(argc, argv, "o:p:r:d:tTv")) != -1)
  {
    switch (opt)
    {
    case 'o':
      output_filename = optarg;
      break;
    case 'p':
      pagenum = atoi(optarg);
      break;
    case 'r':
      dpi = atoi(optarg);
      break;
    case 'd':
      output_dir = optarg;
      break;
    case 't':
      t_flag = 1;
      break;
    case 'T':
      T_flag = 1;
      break;
    case 'v': 
      g_verbose = 1;
      break;
    default: // '?'
      print_usage(argv[0]);
      return (1);
    }
  }

  // Check for the required input filename
  if (optind < argc)
  {
    input_filename = argv[optind];
  }
  else
  {
    fprintf(stderr, "ERROR: Missing input PDF file.\n");
    print_usage(argv[0]);
    return (1);
  }

  // --- Filename and Argument Validation for Render Mode ---
  if (!analyze_mode)
  {
    int output_options_count = (output_filename != NULL) + t_flag + T_flag;
    if (output_options_count > 1)
    {
      fprintf(stderr, "ERROR: Options -o, -t/-d, and -T are mutually exclusive.\n");
      print_usage(argv[0]);
      return (1);
    }

    if (t_flag)
    {
      if (output_dir == NULL)
      {
        fprintf(stderr, "ERROR: The -t option requires the -d <directory> option.\n");
        print_usage(argv[0]);
        return (1);
      }

      char *basename = strrchr(input_filename, '/');
      basename = (basename == NULL) ? input_filename : basename + 1;
      char *dot = strrchr(basename, '.');
      size_t len = (dot == NULL) ? strlen(basename) : (size_t)(dot - basename);
      char name_without_ext[256];
      strncpy(name_without_ext, basename, len);
      name_without_ext[len] = '\0';

      int random_num = rand() % 1000;
      snprintf(temp_output_filename, sizeof(temp_output_filename), "%s/%sResult%03d.png", output_dir, name_without_ext, random_num);
      output_filename = temp_output_filename;
    }
    else if (T_flag)
    {
      const char *fixed_dir = "testfiles/renderer-output";
      char *basename = strrchr(input_filename, '/');
      basename = (basename == NULL) ? input_filename : basename + 1;
      char *dot = strrchr(basename, '.');
      size_t len = (dot == NULL) ? strlen(basename) : (size_t)(dot - basename);
      char name_without_ext[256];
      strncpy(name_without_ext, basename, len);
      name_without_ext[len] = '\0';

      int random_num = rand() % 1000;
      snprintf(temp_output_filename, sizeof(temp_output_filename), "%s/%sResult%03d.png", fixed_dir, name_without_ext, random_num);
      output_filename = temp_output_filename;
    }

    if (output_filename == NULL)
    {
      fprintf(stderr, "ERROR: Missing output filename. Use -o, -t/-d, or -T.\n");
      print_usage(argv[0]);
      return (1);
    }
  }

  // --- PDF File Processing ---

  if ((pdf = pdfioFileOpen(input_filename, NULL, NULL, NULL, NULL)) == NULL)
    return (1);

  if ((page = pdfioFileGetPage(pdf, pagenum - 1)) == NULL)
  {
    fprintf(stderr, "ERROR: Unable to get page %d from '%s'.\n", pagenum, input_filename);
    pdfioFileClose(pdf);
    return (1);
  }

  pdfio_dict_t *page_dict = pdfioObjGetDict(page);
  pdfio_obj_t *resources = pdfioDictGetObj(page_dict, "Resources");
  pdfio_stream_t *st = pdfioPageOpenStream(page, 0, true);

  if (!st)
  {
    fprintf(stderr, "ERROR: Unable to open content stream for page %d.\n", pagenum);
    pdfioFileClose(pdf);
    return (1);
  }

  if (g_verbose)
    printf("DEBUG: Successfully opened PDF and page %d stream.\n", pagenum);

  // --- Main Logic ---

  if (analyze_mode)
  {
    printf("Analyzing page %d of '%s'...\n", pagenum, input_filename);
    analyze_content_stream(st);
    print_and_free_analysis_summary();
  }
  else
  {
    printf("Rendering page %d of '%s' to '%s' at %d DPI...\n", pagenum, input_filename, output_filename, dpi);

    pdfio_dict_t *render_page_dict = pdfioObjGetDict(page);
    if (!pdfioDictGetRect(render_page_dict, "MediaBox", &mediabox))
    {
      fprintf(stderr, "ERROR: Could not get MediaBox for page %d.\n", pagenum);
      pdfioFileClose(pdf);
      pdfioStreamClose(st);
      return (1);
    }
    
    if (g_verbose)
      printf("DEBUG: Page MediaBox is [%f %f %f %f].\n", mediabox.x1, mediabox.y1, mediabox.x2, mediabox.y2);

    p2c_device_t *dev = device_create(mediabox, dpi);
    if (dev)
    {
      if (g_verbose)
        printf("DEBUG: Cairo device created.\n");

      process_content_stream(dev, st, resources);
      
      device_save_to_png(dev, output_filename);
      device_destroy(dev);
      
      if (g_verbose)
        printf("DEBUG: Device destroyed, render complete.\n");
    }
    else
    {
      status = 1; // Device Creation failed
    }
  }

  // --- Clean Up ---
  pdfioStreamClose(st);
  pdfioFileClose(pdf);

  if (g_verbose)
    printf("DEBUG: PDF file closed. Exiting.\n");

  return (status);
}
