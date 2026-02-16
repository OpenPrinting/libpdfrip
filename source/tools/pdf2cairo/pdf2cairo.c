//                                                                                         
// Copyright 2025 Yash Kumar Kasaudhan <vididvidid@gmail.com>                              
// Copyright 2025-2026 Uddhav Phatak <uddhavphatak@gmail.com>                                   
//                                                                                         
// Licensed under Apache License v2.0.  See the file "LICENSE" for more                    
// information.                                                                            
//             

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pdfops-private.h"
#include "../pdf/parser.h"
#include "../cairo/cairo-private.h"

int g_verbose = 0;

//
// 'print_usage()' - Function to show command-line help.
//

static void
print_usage(const char *prog_name) // I - Program name
{
  fprintf(stderr, "Usage: %s [options] input.pdf\n", prog_name);
  fprintf(stderr, "\nOptions:\n");
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
// 'main()' - Main entry for the pdf2cairo
//

int            		  // O - Exit status
main(int argc, 		// I - Number of command-line args
     char **argv) 	// I - Command-line arguments
{
  pdfrip_doc_t 		*PDF_doc;		// PDF meta data structure
						
  // Command-line options
  char 			*input_filename = NULL;		// input PDF filename	
  char 			*output_filename = NULL; 	// output filename
  int 			pagenum = 1;
  size_t 		cur_page;			// page iterator
  int 			dpi = 72;
  int analyze_mode = 0;
  int opt;
 
  // flags
  char *output_dir = NULL;
  int t_flag = 0;
  int T_flag = 0;
  char temp_output_filename[1024]; // Buffer for generated filename


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

  //pdf FIle processing
  PDF_doc = openPDFfile(input_filename);	

  for(cur_page=0; cur_page<PDF_doc->num_pages ; cur_page++)
  {
    pdfrip_page_t *page = getPageData(PDF_doc, cur_page); 
    
    p2c_device_t *dev = device_create(page, dpi);
    if (dev)
    {
      // this sets the current page being worked upon into the context(dev will act as context)
      dev->page = page->object;

      if(!page->resources_dict)
      { 
	fprintf(stderr, "ERROR: PDF file is not correct, No Resource dictionary");
        device_destroy(dev);
        freePageData(page);
        freePDFdoc(PDF_doc);
	return 1;
      }

      // Load the font glyphs to dev
      dev->font_dict = pdfioDictGetDict(page->resources_dict, "Font");
      if(!dev->font_dict)
      {
	//TODO: set a default
        fprintf(stderr, "No font dictionary in Resources dict\n");
      }
      else
      {
        dev->num_fonts = pdfioDictGetNumPairs(dev->font_dict);
	dev->fonts = (p2c_font_t**)calloc(dev->num_fonts, sizeof(p2c_font_t*));

	for (size_t i = 0; i < dev->num_fonts; i++)
	{
  	  // Allocate each individual font structure
  	  dev->fonts[i] = (p2c_font_t *)calloc(1, sizeof(p2c_font_t));
       	}

	if(!getPageFonts(dev))
	{
	  fprintf(stderr, "ERROR: Could not extract Font Glyphs\n");
          device_destroy(dev);
	  freePageData(page);
	  freePDFdoc(PDF_doc);
	  return 1;
	}
      }
      fprintf(stderr, "+++++++++++++++++++++++%ld++++++++\n",dev->num_fonts);

      // Locate the /XObject dictionary and store its reference
      pdfio_obj_t *xobject_res_obj = pdfioDictGetObj(page->resources_dict, "XObject");
      dev->xobject_dict = xobject_res_obj ? pdfioObjGetDict(xobject_res_obj) : NULL;

      process_content_stream(dev, page);
      device_save_to_png(dev, output_filename);
      device_destroy(dev);
    }
    freePageData(page);
  }

  fprintf(stderr, "%s\n", PDF_doc->version);
  freePDFdoc(PDF_doc);

  return 0;
}
