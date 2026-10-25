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
  fprintf(stderr, "                         A multi-page PDF writes one PNG per page:\n");
  fprintf(stderr, "                         output-1.png, output-2.png, ...\n");
  fprintf(stderr, "  -p <pagenum>           Render only this page, to the exact output filename\n");
  fprintf(stderr, "                         (default: every page).\n");
  fprintf(stderr, "  -r <dpi>               Specify the resolution in DPI (default: 72).\n");
  fprintf(stderr, "  -t                     Generate an output filename from the input name (e.g., 'input.png').\n");
  fprintf(stderr, "                         Must be used with the -d option.\n");
  fprintf(stderr, "  -d <directory>         Specify the output directory when using -t.\n");
  fprintf(stderr, "  -T                     Generate a temporary filename in 'testfiles/renderer-output/'.\n");
  fprintf(stderr, "  -v                     Enable verbose debugging output.\n"); 
}

//
// 'page_filename()' - Name the PNG of one page of a multi-page render.
//

static void
page_filename(char       *buffer,	// O - Page filename
              size_t     bufsize,	// I - Size of buffer
              const char *filename,	// I - Output filename
              size_t     page,		// I - Page number (1-based)
              size_t     num_pages)	// I - Number of pages
{
  const char	*base = strrchr(filename, '/');
  const char	*ext;
  int		width = snprintf(NULL, 0, "%zu", num_pages);

  base = base ? base + 1 : filename;

  // Insert the page number before the extension, if there is one
  if ((ext = strrchr(base, '.')) == NULL || ext == base)
    ext = filename + strlen(filename);

  snprintf(buffer, bufsize, "%.*s-%0*zu%s", (int)(ext - filename), filename,
           width, page, ext);
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
  char			page_output[1100];		// output filename of one page
  int 			pagenum = 0;			// page to render, 0 = all
  size_t 		cur_page,			// page iterator
			first_page, last_page;		// pages to render (0-based)
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
      if (pagenum < 1)
      {
        fprintf(stderr, "ERROR: Invalid page number '%s'.\n", optarg);
        print_usage(argv[0]);
        return (1);
      }
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
      snprintf(temp_output_filename, sizeof(temp_output_filename), "%s/%s.png", output_dir, name_without_ext);
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
      snprintf(temp_output_filename, sizeof(temp_output_filename), "%s/%s.png", fixed_dir, name_without_ext);
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
  if (!PDF_doc)
    return (1);

  if ((size_t)pagenum > PDF_doc->num_pages)
  {
    fprintf(stderr, "ERROR: Page %d does not exist, the PDF has %zu page(s).\n",
            pagenum, PDF_doc->num_pages);
    freePDFdoc(PDF_doc);
    return (1);
  }

  first_page = pagenum ? (size_t)pagenum - 1 : 0;
  last_page  = pagenum ? (size_t)pagenum - 1 : PDF_doc->num_pages - 1;

  for(cur_page=first_page; cur_page<=last_page ; cur_page++)
  {
    pdfrip_page_t *page = getPageData(PDF_doc, cur_page); 
    
    p2c_device_t *dev = device_create(page, dpi);
    if (dev)
    {
      // this sets the current page being worked upon into the context(dev will act as context)
      dev->page_obj = page->object;

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
      pdfio_valtype_t font_structure = pdfioDictGetType(page->resources_dict, "Font");
      if(!font_structure)
      {
	//TODO: set a default
        fprintf(stderr, "No font dictionary in Resources dict\n");
      }
      else
      {
        if(font_structure == PDFIO_VALTYPE_INDIRECT)
	   dev->font_dict = pdfioObjGetDict(pdfioDictGetObj(page->resources_dict, "Font"));
        else if(font_structure == PDFIO_VALTYPE_DICT)
	   dev->font_dict = pdfioDictGetDict(page->resources_dict, "Font");

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

      // Rendering several pages writes one PNG per page
      if (output_filename && first_page == last_page)
        device_save_to_png(dev, output_filename);
      else if (output_filename)
      {
        page_filename(page_output, sizeof(page_output), output_filename,
                      cur_page + 1, PDF_doc->num_pages);
        device_save_to_png(dev, page_output);
      }

      device_destroy(dev);
    }
    freePageData(page);
  }

  fprintf(stderr, "%s\n", PDF_doc->version);
  freePDFdoc(PDF_doc);

  return 0;
}
